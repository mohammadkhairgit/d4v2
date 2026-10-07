#!/usr/bin/env python3
"""Run d4 for every requested input/configuration combination."""

from itertools import product
from datetime import datetime
import os
from pathlib import Path
import signal
import subprocess
import time





DESCRIPTION = """DESCRIPTION of task: """

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
D4_BINARY = REPOSITORY_ROOT / "build" / "d4"
DDNNFIFE_BINARY = REPOSITORY_ROOT / "build" / "ddnnife"
INPUT_DIRECTORY = REPOSITORY_ROOT / "instancesTest" / "cnfs_and_alternatives" / "extra" / "inputs" / "car_confidential" / "dimacsOutputPath"
RESULT_DIRECTORY = REPOSITORY_ROOT / "instancesTest" / "cnfs_and_alternatives" / "extra" / "results" / "car_confidential" / "dimacsOutputPath"
RESULT_DIRECTORY.mkdir(parents=True, exist_ok=True)
output_file = RESULT_DIRECTORY / "to_ddnnf_output.log"
output_ddnnf_file = RESULT_DIRECTORY / "ddnnf_statistics_output.log"
run_started_at = datetime.now().isoformat(timespec="seconds")
with output_file.open("a") as log1:
    log1.write(f"\nDate: {run_started_at}\n")
    log1.write(f"{DESCRIPTION}\n")
    log1.write("Starting d4 runs...\n")
with output_ddnnf_file.open("a") as log2:
    log2.write(f"\nDate: {run_started_at}\n")
    log2.write(f"{DESCRIPTION}\n")
    log2.write("Starting ddnnife runs...\n")

# Edit these arrays to select the runs for a batch.
FILE_NAMES = [
    # "1-Car_with_all_features_unsliced",
    # "1-Car_with_all_features_unsliced (Copy)",
    # "2-Car_with_all_features_unsliced",
    # "3-Car_with_all_features_unsliced",
    # "4-Car_with_all_features_unsliced",
    # "5-Car_with_all_features_unsliced",
    # "6-Car_with_all_features_unsliced",
    # "7-Car_with_all_features_unsliced",
    # "8-Car_with_all_features_unsliced",
    # "Car_with_all_features_unsliced_1d9bb9d8-89e3-4d42-9841-c224c1e7df60",
    # "Car_with_all_features_unsliced_2f8cf3af-e8d4-43cd-bcba-c9a4736459c1",
    # "Car_with_all_features_unsliced_3a57834f-0c66-4e63-a4ed-b0c0e7b05480",
    # "Car_with_all_features_unsliced_4703888f-3fd4-4b86-b6cc-5c3b8395426c",
    # "Car_with_all_features_unsliced_526e7e15-011e-40bd-910f-b496b8094c58",
    # "Car_with_all_features_unsliced_8c3c50e6-3381-4b5c-81ac-6336db94f497",
    # "Car_with_all_features_unsliced_d5e97e2a-928e-4e58-aef6-57d07d77cec8",
    # "Car_with_all_features_unsliced_d65c9472-1e79-4338-acd4-1dc49a2cc0a0",
    # "FinancialServices01-Nieke2018-2018-03-26",
    # "FinancialServices01-Nieke2018-2018-04-23",
    # "FinancialServices01-Nieke2018-2018-05-09",
    # "automotive01-Kowal2016",
    # "automotive2-Knüppel2017-2_1",
    # "automotive2-Knüppel2017-2_2",
    # "automotive2-Knüppel2017-2_3",
    # "automotive2-Knüppel2017-2_4",
    # "cdl-adder-Knüppel2017",
    # "cdl-adder-Knüppel2017 copy",
    # "cdl-aim711-Knüppel2017",
    # "embtoolkit-Knüppel2017",
    # "firefoxcve-VarelaVaca2020-2002-2436",
    # "firefoxcve-VarelaVaca2020-2009-1308",
    # "firefoxcve-VarelaVaca2020-2011-1300",
    # "firefoxcve-VarelaVaca2020-2012-0471",
    # "firefoxcve-VarelaVaca2020-2012-3987",
    # "firefoxcve-VarelaVaca2020-2013-0783",
    # "firefoxcve-VarelaVaca2020-2015-0819",
    # "firefoxcve-VarelaVaca2020-2017-5393",
    # "firefoxcve-VarelaVaca2020-2020-12396",
    # "linux-Knüppel2017-2.6.33.3",
    # "pc-richmond-Sprey2020",
]
CLAUSE_SCORING_ARRAY = [
                "none",
                # "literal-sum",
                # "literal-sum-over-branch-avg",
                # "literal-min",
                # "branch-min",
                # "literal-max",
                # "branch-max",
                # "branches-avg-divided-by-distribution"
                ]
NO_ALTERNATIVE_SCORING_METHOD_ARRAY = ["mom"]
CACHE_ACTIVATED_BOOL = [False]
SCORING_METHOD_ARRAY = ["mom"]
ALTERNATIVE_INPUT = [True]
NO_CLAUSE_SCORING = ["none"]
RAM_USAGE_LIMIT = 99.0
RAM_USAGE_START_LIMIT = 40.0
MAX_RUNTIME_SECONDS = 10 * 60


def ram_usage_percent() -> float:
    memory = {}
    with Path("/proc/meminfo").open() as meminfo:
        for line in meminfo:
            key, value, *_ = line.split()
            memory[key.rstrip(":")] = int(value)
    total = memory["MemTotal"]
    available = memory["MemAvailable"]
    return (total - available) / total * 100


def wait_for_memory_capacity() -> None:
    while ram_usage_percent() > RAM_USAGE_START_LIMIT:
        print(
            f"RAM usage is above {RAM_USAGE_START_LIMIT:.0f}%; waiting before starting the next subprocess.",
            flush=True,
        )
        time.sleep(60)


def terminate_process_group(process: subprocess.Popen[str]) -> None:
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        return

    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        pass

    # The parent can exit while descendants in the process group remain alive.
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    process.wait()


def run_with_memory_limit(
    command: list[str], output_file_handle, run_details: dict[str, object]
) -> None:
    command_text = " ".join(command)
    print("\n \n \n \n Run configuration:", flush=True)
    for name, value in run_details.items():
        print(f"{name}: {value}", flush=True)
    print(f"command: {command_text}", flush=True)
    output_file_handle.write("\n\nRun configuration:\n")
    for name, value in run_details.items():
        output_file_handle.write(f"{name}: {value}\n")
    output_file_handle.write(f"command: {command_text}\n")
    output_file_handle.flush()
    wait_for_memory_capacity()
    process = subprocess.Popen(
        command,
        cwd=REPOSITORY_ROOT,
        stdout=output_file_handle,
        stderr=subprocess.STDOUT,
        text=True,
        start_new_session=True,
    )
    output_file_handle.write(f"\nSTARTED: {command_text}\n")
    output_file_handle.flush()
    start_time = time.monotonic()
    try:
        while process.poll() is None:
            ram_usage = ram_usage_percent()
            runtime = time.monotonic() - start_time
            if ram_usage >= RAM_USAGE_LIMIT or runtime >= MAX_RUNTIME_SECONDS:
                reason = (
                    f"RAM usage reached {RAM_USAGE_LIMIT:.0f}%"
                    if ram_usage >= RAM_USAGE_LIMIT
                    else f"runtime reached {MAX_RUNTIME_SECONDS // 60} minutes"
                )
                print(
                    f"{reason}; terminating the subprocess.",
                    flush=True,
                )
                output_file_handle.write(
                    f"TERMINATED: {reason}; command: {command_text}\n"
                )
                output_file_handle.flush()
                terminate_process_group(process)
                raise subprocess.CalledProcessError(process.returncode, command)
            time.sleep(5)

        if process.returncode != 0:
            output_file_handle.write(
                f"FAILED: subprocess exited with code {process.returncode}; "
                f"command: {command_text}\n"
            )
            output_file_handle.flush()
            raise subprocess.CalledProcessError(process.returncode, command)
        output_file_handle.write(f"SUCCESS: command completed: {command_text}\n")
        output_file_handle.flush()
    except KeyboardInterrupt:
        output_file_handle.write(
            f"TERMINATED: interrupted by Ctrl+C; command: {command_text}\n"
        )
        output_file_handle.flush()
        terminate_process_group(process)
        raise
    finally:
        terminate_process_group(process)
        runtime = time.monotonic() - start_time
        print(f"Subprocess runtime: {runtime:.2f} seconds", flush=True)

def build_command_ddnnife(
    file_name: str,
    clause_scoring: str,
    cache_activated: bool,
    scoring_method: str,
    alternative_input: bool,
    projected: bool = True,
) -> list[str]:
    output_name = (
        f"{file_name}_cache_{str(cache_activated).lower()}"
        f"_clauseScoring_{clause_scoring}_literalScoring_{scoring_method}_alternative_{str(alternative_input).lower()}_projected_{str(projected).lower()}.ddnnf"
    )
    command = [str(DDNNFIFE_BINARY), "-i", str(RESULT_DIRECTORY / output_name), "count"]
    return command

def build_command(
    file_name: str,
    clause_scoring: str,
    cache_activated: bool,
    scoring_method: str,
    alternative_input: bool,
    projected: bool = True,
) -> list[str]:
    if alternative_input:
        input_file = INPUT_DIRECTORY / f"{file_name}" / "cnf.dimacs"
    else:
        input_file = INPUT_DIRECTORY / f"{file_name}" / "ref.dimacs"
    output_name = (
        f"{file_name}_cache_{str(cache_activated).lower()}"
        f"_clauseScoring_{clause_scoring}_literalScoring_{scoring_method}_alternative_{str(alternative_input).lower()}_projected_{str(projected).lower()}.ddnnf"
    )

    command = [
        str(D4_BINARY),
        "--input",
        str(input_file),
    ]
    if alternative_input:
        command.extend(["--alternative-input", str(INPUT_DIRECTORY / f"{file_name}" / "cnf.alt")])
        command.extend(["--clause-scoring", clause_scoring])
    command.extend(
        [
            "--occurrence-manager",
            "dynamic",
            "--input-type",
            "cnf",
            "--dump-ddnnf",
            str(RESULT_DIRECTORY / output_name),
            "--scoring-method",
            scoring_method,
            "--phase-heuristic",
            "false",
            "--cache-activated",
            str(cache_activated).lower(),
        ]
    )
    if projected:
        command.extend(["-m", "proj-ddnnf-compiler"])
    else:
        command.extend(["-m", "ddnnf-compiler"])
    return command


def main() -> None:
    configurations = product(
        FILE_NAMES,
        CLAUSE_SCORING_ARRAY,
        CACHE_ACTIVATED_BOOL,
        SCORING_METHOD_ARRAY,
        ALTERNATIVE_INPUT,
    )
    alternatives_part_configurations = list(
        product(CLAUSE_SCORING_ARRAY, ALTERNATIVE_INPUT)
    )
    configurations_no_alternatives = product(
        FILE_NAMES,
        CACHE_ACTIVATED_BOOL,
        NO_ALTERNATIVE_SCORING_METHOD_ARRAY,
    )

    for file_name, cache_activated, scoring_method in configurations_no_alternatives:

            
    # for file_name, clause_scoring, cache_activated, scoring_method, alternative_input in configurations:
        for clause_scoring, alternative_input in alternatives_part_configurations:
            command = build_command(
                file_name,
                clause_scoring, 
                cache_activated,
                scoring_method,
                alternative_input,
                False
            )
            try:
                with output_file.open("a") as log1:
                    run_with_memory_limit(
                        command,
                        log1,
                        {
                            "file name": file_name,
                            "cache activated": cache_activated,
                            "scoring method": scoring_method,
                            "clause scoring": clause_scoring,
                            "alternative input": alternative_input,
                            "projected": False,
                        },
                    )
            except subprocess.CalledProcessError as error:
                print(
                    f"d4 failed with exit code {error.returncode}; stopping the batch.",
                    flush=True,
                )
                # raise SystemExit(error.returncode) from error
            
            command = build_command_ddnnife(
                file_name,
                clause_scoring, 
                cache_activated,
                scoring_method,
                alternative_input,
                False
            )
            try:
                with output_ddnnf_file.open("a") as log2:
                    log2.write("\n \n \n \n")
                    log2.write("file name: {}\n".format(file_name))
                    log2.write("cache activated: {}\n".format(cache_activated))
                    log2.write("scoring method: {}\n".format(scoring_method))
                    log2.write("clause scoring: {}\n".format(clause_scoring))
                    log2.write("alternative input: {}\n".format(alternative_input))
                    run_with_memory_limit(
                        command,
                        log2,
                        {
                            "file name": file_name,
                            "cache activated": cache_activated,
                            "scoring method": scoring_method,
                            "clause scoring": clause_scoring,
                            "alternative input": alternative_input,
                            "projected": False,
                        },
                    )
            except subprocess.CalledProcessError as error:
                print(
                    f"d4 failed with exit code {error.returncode}; stopping the batch.",
                    flush=True,
                )
                # raise SystemExit(error.returncode) from error


        command = build_command(
            file_name,
            "none",
            cache_activated,
            scoring_method,
            False,
            False
        )
        try:
            with output_file.open("a") as log1:
                run_with_memory_limit(
                    command,
                    log1,
                    {
                        "file name": file_name,
                        "cache activated": cache_activated,
                        "scoring method": scoring_method,
                        "clause scoring": "none",
                        "alternative input": False,
                        "projected": False,
                    },
                )
        except subprocess.CalledProcessError as error:
            print(
                f"d4 failed with exit code {error.returncode}; stopping the batch.",
                flush=True,
            )
            # raise SystemExit(error.returncode) from error

        command = build_command_ddnnife(
            file_name,
            "none",
            cache_activated,
            scoring_method,
            False,
            False
        )
        try:
            with output_ddnnf_file.open("a") as log2:
                log2.write("\n \n \n \n")
                log2.write("file name: {}\n".format(file_name))
                log2.write("cache activated: {}\n".format(cache_activated))
                log2.write("scoring method: {}\n".format(scoring_method))
                log2.write("alternative input: {}\n".format(False))
                run_with_memory_limit(
                    command,
                    log2,
                    {
                        "file name": file_name,
                        "cache activated": cache_activated,
                        "scoring method": scoring_method,
                        "clause scoring": "none",
                        "alternative input": False,
                        "projected": False,
                    },
                )
        except subprocess.CalledProcessError as error:
            print(
                f"d4 failed with exit code {error.returncode}; stopping the batch.",
                flush=True,
            )
            # raise SystemExit(error.returncode) from error
    

if __name__ == "__main__":
    main()