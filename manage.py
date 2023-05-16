import openai
import os
import shutil
import argparse
import datetime
import traceback
import subprocess
openai.api_key = os.getenv('OPENAI_API_KEY')

PROMPT_SUFFIX = ""
SCRIPT_FOLDER_PATH = os.path.dirname(os.path.realpath(__file__))
BUILD_FOLDER_PATH = os.path.join(SCRIPT_FOLDER_PATH, "build")


def get_file_content(file_path: str):
    with open(file_path, 'r') as f:
        return f.read()


def write_source_code():
    current_datetime = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    uname_output = subprocess.run(['uname', '-a'], capture_output=True, text=True).stdout.strip()

    state_content = f"{current_datetime}\n{uname_output}\n"
    os.chdir(SCRIPT_FOLDER_PATH)

    file_candidates = os.listdir('.') + [os.path.join(SCRIPT_FOLDER_PATH, '.github/workflows/main.yml')]

    for file_name in file_candidates:
        if file_name.endswith('.py') or file_name.endswith('CMakeLists.txt') or file_name.endswith('.cpp') or file_name.endswith('.h') or file_name.endswith('requirements.txt'):
            state_content += f"--- {file_name} ---"
            state_content += get_file_content(file_name)
            state_content += "\n\n"

    with open('state.txt', 'w') as f:
        f.write(state_content)


def write_state_file(config_output_msg: str, build_output_msg: str, config_command: str, build_command: str):
    write_source_code()
    state_content = ""
    state_content += f"{config_command}\n"
    state_content += f"{config_output_msg}\n"
    state_content += f"{build_command}\n"
    state_content += f"{build_output_msg}\n"

    os.chdir(SCRIPT_FOLDER_PATH)
    with open("state.txt", "a") as f:
        f.write(state_content)


def build() -> bool:
    try:
        # NOTE: since we don't cd to SCRIPT_FOLDER_PATH, this should work for any cwd
        if os.path.exists('build'):
            shutil.rmtree('build')

        if not os.path.exists("build"):
            subprocess.run(["mkdir", "build"], check=True)

        os.chdir("build")
        config_command = ["cmake", "..", "-DCMAKE_BUILD_TYPE=Debug"]
        config_result = subprocess.run(config_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        build_command = ["cmake", "--build", "."]
        build_result = subprocess.run(build_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        _build_success = config_result.returncode == 0 and build_result.returncode == 0
        if not _build_success:
            config_msg = f"stdout:\n{config_result.stdout}\nstderr:\n"
            build_msg = f"stdout:\n{build_result.stdout}\nstderr:\n"
        else:
            config_msg = ''
            build_msg = ''

        write_state_file(config_msg, build_msg, ' '.join(config_command), ' '.join(build_command))
        return _build_success

    except Exception as e:
        print(f"Error: {e}")
        formatted_traceback = traceback.format_exc()
        write_source_code()
        with open('state.txt', 'a') as f:
            f.write(f"\n{formatted_traceback}")


def run_tests() -> bool:
    try:
        test_command = [os.path.join(BUILD_FOLDER_PATH, 'tests')]
        test_result = subprocess.run(test_command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        _test_success = test_result.returncode == 0

        if not _test_success:
            test_msg = f"stdout:\n{test_result.stdout}\nstderr:\n{test_result.stderr}\n"
            with open('state.txt', 'a') as f:
                f.write(test_msg)
        else:
            with open("state.txt", 'a') as f:
                f.write("Tests passed successfully!")

        return _test_success

    except Exception as e:
        print(f"Error: {e}")
        formatted_traceback = traceback.format_exc()
        write_source_code()
        with open('state.txt', 'a') as f:
            f.write(f"\n{formatted_traceback}")


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument("binary", help="Binary target", type=str)
    parser.add_argument("--send", help="Send the build context and results to GPT", action="store_true")
    args = parser.parse_args()

    build_success = build()
    test_success = run_tests() if build_success else False

    if args.send:
        os.chdir(SCRIPT_FOLDER_PATH)
        prompt = open('prompt.txt').read()
        state = open('state.txt').read()

        messages = [
            {
                'role': 'system',
                'content': state
            },
            {
                'role': 'user',
                'content': prompt
            }
        ]

        print(f"{prompt}")
        response = openai.ChatCompletion.create(model='gpt-4', messages=messages, stream=True)

        answer = ''
        timestamp = datetime.datetime.now().strftime('%H_%M_%S')
        response_filename = f"response_{timestamp}.md"

        with open(response_filename, 'w') as response_file:
            for chunk in response:
                try:
                    s = chunk['choices'][0]['delta']['content']

                    answer += s
                    print(s, end='')
                    response_file.write(s)
                    response_file.flush()
                except:
                    pass
            print('\n')
