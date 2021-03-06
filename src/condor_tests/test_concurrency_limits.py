#!/usr/bin/env pytest

# this test replicates job_concurrency_limitsP

import re
import logging

from ornithology import (
    config,
    standup,
    action,
    Condor,
    SetJobStatus,
    JobStatus,
    in_order,
    track_quantity,
    SCRIPTS,
)

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

# the effective number of slots should always be much larger than the number of
# jobs you plan to submit
SLOT_CONFIGS = {
    "static_slots": {"NUM_CPUS": "12", "NUM_SLOTS": "12"},
    "partitionable_slot": {
        "NUM_CPUS": "12",
        "SLOT_TYPE_1": "cpus=100%,memory=100%,disk=100%",
        "SLOT_TYPE_1_PARTITIONABLE": "True",
        "NUM_SLOTS_TYPE_1": "1",
    },
}


@config(params=SLOT_CONFIGS)
def slot_config(request):
    return request.param


@standup
def condor(test_dir, slot_config):
    # set all of the concurrency limits for each slot config,
    # so that we can run all the actual job submits against the same config
    concurrency_limit_config = {
        v["config-key"]: v["config-value"] for v in CONCURRENCY_LIMITS.values()
    }

    with Condor(
        local_dir=test_dir / "condor",
        config={
            **slot_config,
            **concurrency_limit_config,
            # The negotiator determines if a concurrency limit is in use by
            # checking the machine ads, so it will overcommit tokens if its
            # cycle time is shorter than the claim-and-report cycle.
            #
            # I'm not sure why the claim-and-report cycle is so long.
            "NEGOTIATOR_INTERVAL": "4",
            "NEGOTIATOR_CYCLE_DELAY": "4",
        },
    ) as condor:
        yield condor


CONCURRENCY_LIMITS = {
    "named_limit": {
        "config-key": "XSW_LIMIT",
        "config-value": "4",
        "submit-value": "XSW",
        "max-running": 4,
    },
    "default_limit": {
        "config-key": "CONCURRENCY_LIMIT_DEFAULT",
        "config-value": "2",
        "submit-value": "UNDEFINED:2",
        "max-running": 1,
    },
    "default_small": {
        "config-key": "CONCURRENCY_LIMIT_DEFAULT_SMALL",
        "config-value": "3",
        "submit-value": "small.license",
        "max-running": 3,
    },
    "default_large": {
        "config-key": "CONCURRENCY_LIMIT_DEFAULT_LARGE",
        "config-value": "1",
        "submit-value": "large.license",
        "max-running": 1,
    },
}


@action(params=CONCURRENCY_LIMITS)
def concurrency_limit(request):
    return request.param


@action
def all_jobs_ran(condor, concurrency_limit, path_to_sleep):
    handle = condor.submit(
        description={
            "executable": path_to_sleep,
            "arguments": "5",
            "request_memory": "100MB",
            "request_disk": "10MB",
            "concurrency_limits": concurrency_limit["submit-value"],
        },
        count=(concurrency_limit["max-running"] + 1) * 2,
    )

    condor.job_queue.wait_for_events(
        {
            jobid: [
                (
                    SetJobStatus(JobStatus.RUNNING),
                    lambda j, e: condor.run_command(["condor_q"], echo=True),
                ),
                SetJobStatus(JobStatus.COMPLETED),
            ]
            for jobid in handle.job_ids
        },
        # On my unloaded machine, it takes ~48 seconds for the longest test.
        timeout=180,
    )

    yield handle

    handle.remove()


@action
def num_jobs_running_history(condor, all_jobs_ran, concurrency_limit):
    return track_quantity(
        condor.job_queue.filter(lambda j, e: j in all_jobs_ran.job_ids),
        increment_condition=lambda id_event: id_event[-1]
        == SetJobStatus(JobStatus.RUNNING),
        decrement_condition=lambda id_event: id_event[-1]
        == SetJobStatus(JobStatus.COMPLETED),
        max_quantity=concurrency_limit["max-running"],
        expected_quantity=concurrency_limit["max-running"],
    )


@action
def startd_log_file(condor):
    return condor.startd_log.open()


@action
def num_busy_slots_history(startd_log_file, all_jobs_ran, concurrency_limit):
    logger.debug("Checking Startd log file...")
    logger.debug("Expected Job IDs are: {}".format(all_jobs_ran.job_ids))

    active_claims_history = track_quantity(
        startd_log_file.read(),
        increment_condition=lambda msg: "Changing activity: Idle -> Busy" in msg,
        decrement_condition=lambda msg: "Changing activity: Busy -> Idle" in msg,
        max_quantity=concurrency_limit["max-running"],
        expected_quantity=concurrency_limit["max-running"],
    )

    return active_claims_history


@action
def negotiator_log(all_jobs_ran, condor):
    return condor.negotiator_log.open()


@action
def concurrency_limits_hit(negotiator_log):
    limits_hit = dict()
    for entry in negotiator_log.read():
        match = re.match(
            r"^\s*Rejected .*: concurrency limit (.*) reached$", entry.message
        )
        if match:
            limit = match.group(1).lower()
            value = limits_hit.setdefault(limit, 0)
            limits_hit[limit] = value + 1
    return limits_hit


class TestConcurrencyLimits:
    def test_all_jobs_ran(self, condor, all_jobs_ran):
        for jobid in all_jobs_ran.job_ids:
            assert in_order(
                condor.job_queue.by_jobid[jobid],
                [
                    SetJobStatus(JobStatus.IDLE),
                    SetJobStatus(JobStatus.RUNNING),
                    SetJobStatus(JobStatus.COMPLETED),
                ],
            )

    def test_never_more_jobs_running_than_limit(
        self, num_jobs_running_history, concurrency_limit
    ):
        assert max(num_jobs_running_history) <= concurrency_limit["max-running"]

    def test_num_jobs_running_hits_limit(
        self, num_jobs_running_history, concurrency_limit
    ):
        assert concurrency_limit["max-running"] in num_jobs_running_history

    def test_never_more_busy_slots_than_limit(
        self, num_busy_slots_history, concurrency_limit
    ):
        assert max(num_busy_slots_history) <= concurrency_limit["max-running"]

    def test_num_busy_slots_hits_limit(self, num_busy_slots_history, concurrency_limit):
        assert concurrency_limit["max-running"] in num_busy_slots_history

    def test_negotiator_hits_limit(self, concurrency_limit, concurrency_limits_hit):
        limit_name_in_log = concurrency_limit["submit-value"].lower().split(":")[0]
        assert limit_name_in_log in concurrency_limits_hit
