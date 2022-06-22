add_test( Step3Tester.MakeGrid /workspaces/fem-with-dealii-2022/labs/lab-03/build/gtest [==[--gtest_filter=Step3Tester.MakeGrid]==] --gtest_also_run_disabled_tests)
set_tests_properties( Step3Tester.MakeGrid PROPERTIES WORKING_DIRECTORY /workspaces/fem-with-dealii-2022/labs/lab-03/build)
set( gtest_TESTS Step3Tester.MakeGrid)
