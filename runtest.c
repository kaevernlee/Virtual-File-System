#include <stdio.h>

#define TEST(x) test(x, #x)
#include "myfilesystem.h"

/* You are free to modify any part of this file. The only requirement is that when it is run, all your tests are automatically executed */

/* Some example unit test functions */
int success() {
    return 0;
}

int failure() {
    return 1;
}

int no_operation() {
    void * helper = init_fs("file1", "file2", "file3", 1); // Remember you need to provide your own test files and also check their contents as part of testing
    close_fs(helper);
    return 0;
}

int test_create_file() {
    void * helper = init_fs("03_file_data", "03_directory_table", "03_hash_data", 4);
    print_dir(helper);
    close_fs(helper);
    return 0;
}

int test_combine() {
    void * helper = init_fs("03_file_data", "03_directory_table", "03_hash_data", 4);
    print_dir(helper);
    tester_add(helper, "new_file!", 350, 360);
    print_dir(helper);
    combine(helper, "new_file!");
    print_dir(helper);
    close_fs(helper);
    return 0;

}
/****************************/

/* Helper function */
void test(int (*test_function) (), char * function_name) {
    int ret = test_function();
    if (ret == 0) {
        printf("Passed %s\n", function_name);
    } else {
        printf("Failed %s returned %d\n", function_name, ret);
    }
}
/************************/

int main(int argc, char * argv[]) {
    
    // You can use the TEST macro as TEST(x) to run a test function named "x"
    TEST(success);
    TEST(failure);
    TEST(no_operation);
    TEST(test_create_file);
    TEST(test_combine);
    // Add more tests here

    return 0;
}
