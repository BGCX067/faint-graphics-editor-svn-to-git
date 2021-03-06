import os

def file_name_to_function_call(f):
    return f.replace('.cpp', '();').replace('-','_')

def file_name_to_function_pointer(f):
    return f.replace('.cpp', '').replace('-','_')

def file_name_to_declaration(f):
    return 'void ' + f.replace('.cpp', '();').replace('-','_')

def run(root_dir, out_file):
    files = [f for f in os.listdir(root_dir) if f.endswith(".cpp") and f != "test-runner.cpp"]

    max_width = max([len(f) for f in files])
    out_dir = os.path.dirname(out_file)
    if not os.path.exists(out_dir):
        os.mkdir(out_dir)
    with open(out_file, 'w') as out:
        out.write('// Generated by %s\n' % os.path.basename(__file__))
        out.write('#include <iostream>\n');
        out.write('#include <iomanip>\n');
        out.write('#include <sstream>\n');
        out.write('#include "test-sys/test-sys.hh"\n')
        out.write('\n');

        out.write('bool TEST_FAILED = false;\n')
        out.write('std::stringstream TEST_OUT;\n')

        for f in files:
            out.write('%s\n' % file_name_to_declaration(f))
        out.write('\n')
        out.write('int main(int, char**){\n');
        out.write('  int numFailed = 0;\n')

        for f in files:
            func = file_name_to_function_pointer(f)
            pad = max_width - len(f)
            out.write('  run_test(%s, "%s", %d, numFailed);\n' % (func, f, max_width))

        out.write('  return print_test_summary(numFailed);\n')
        out.write('}\n')
