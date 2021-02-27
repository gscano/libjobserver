#!/bin/bash

cmd=$1

declare -A pages=(
    ["jobserver_getenv.3"]="env.3"
    ["jobserver_setenv.3"]="env.3"
    ["jobserver_unsetenv.3"]="env.3"

    ["jobserver_read_env_.3"]="env_.3"
    ["jobserver_write_env_.3"]="env_.3"

    ["jobserver_connect.3"]="init.3"
    ["jobserver_reconnect.3"]="init.3"
    ["jobserver_create.3"]="init.3"
    ["jobserver_create_n.3"]="init.3"
    ["jobserver_close.3"]="init.3"

    ["jobserver_wait.3"]="wait.3"
    ["jobserver_collect.3"]="wait.3"

    ["jobserver_launch_job.3"]="handle.3"
    ["jobserver_clear.3"]="handle.3"

    ["jobserver_terminate_job.3"]="handle_.3"
)

for page in "${!pages[@]}"
do
    $cmd ${pages[$page]} $page
done
