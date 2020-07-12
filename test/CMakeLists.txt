function(UnitTest test_file depends)
    get_filename_component(target_name ${test_file} NAME_WE)
    add_executable(${target_name} ${test_file})

    target_include_directories(${target_name} PRIVATE ${PROJECT_SOURCE_DIR}/include)

    foreach(dep ${depends})
        target_link_libraries(${target_name} ${dep})
    endforeach()
    
    add_test(${target_name} ${target_name})
endfunction(UnitTest)

UnitTest(rbhash_allocator.cc "rbhash;gtest")
UnitTest(rbhash_component.cc "rbhash;gtest")
UnitTest(rbhash_construct.cc "rbhash;gtest")
UnitTest(rbhash_iter.cc "rbhash;gtest")
UnitTest(rbhash_operation.cc "rbhash;gtest")
UnitTest(rbhash_stress.cc "rbhash;gtest")