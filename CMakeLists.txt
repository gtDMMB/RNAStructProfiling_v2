cmake_minimum_required(VERSION 3.10)
project(RNAStructProfiling_master C)

set(CMAKE_C_STANDARD 11)

include_directories(.)
include_directories(include)

add_executable(RNAStructProfiling_master
        include/AdvancedDouble.h
        include/algorithms-partition.h
        include/algorithms.h
        include/boltzmann_main.h
        include/constants.h
        include/constraints.h
        include/data.h
        include/energy.h
        include/global.h
        include/gtfold_config.h
        include/key.h
        include/loader.h
        include/main-c.h
        include/main.h
        include/mfe_main.h
        include/options.h
        include/partition-dangle.h
        include/partition-func-d2.h
        include/partition-func.h
        include/pf-shel-check.h
        include/random-sample.h
        include/shapereader.h
        include/stochastic-sampling-d2.h
        include/stochastic-sampling.h
        include/subopt_main.h
        include/subopt_traceback.h
        include/traceback.h
        include/utils-c.h
        include/utils.h
        graph.c
        graph.h
        hashtbl.c
        hashtbl.h
        helix_class.c
        helix_class.h
        main.c
        memoryDFS.c
        memoryDFS.h
        Options.c
        Options.h
        Profile.c
        Profile.h
        Profnode.c
        Profnode.h
        Set.c
        Set.h stem.h stem.c
        array_list.c array_list.h list.h)
