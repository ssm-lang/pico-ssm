add_library(ssm-runtime OBJECT
    ssm-runtime/src/ssm-closure.c
    ssm-runtime/src/ssm-mem.c
    ssm-runtime/src/ssm-scheduler.c
    ssm-runtime/src/ssm-top-act.c
)
target_include_directories(ssm-runtime PRIVATE ssm-runtime/include)
target_include_directories(ssm-runtime INTERFACE ssm-runtime/include)
