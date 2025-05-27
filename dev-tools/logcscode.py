#!/usr/bin/env python3
import os
import sys
import io # For capturing stdout
import subprocess # For calling clipboard utilities
import platform # For OS detection

# --- Configuration ---
GAME_DIR_BASENAME = "game"
EXCLUDE_DIR_BASENAMES = ["bin", "obj"]
# --- End Configuration ---

def log_directory_structure(start_abs_path, game_dir_actual_basename, exclude_abs_paths):
    """
    Logs the directory structure (including files) starting from start_abs_path.
    (Prints to current sys.stdout, which will be captured)
    """
    print(f"Directory Structure for: {game_dir_actual_basename}")
    print("==========================================")

    for root, dirs, files in os.walk(start_abs_path, topdown=True):
        dirs[:] = [d for d in dirs if os.path.join(root, d) not in exclude_abs_paths]
        relative_to_start = os.path.relpath(root, start_abs_path)
        
        if relative_to_start == ".":
            level = 0
            current_dir_display_name = game_dir_actual_basename
        else:
            current_dir_display_name = os.path.basename(root)
            level = len(os.path.normpath(relative_to_start).split(os.sep))
            
        dir_indent = "    " * level
        print(f"{dir_indent}{current_dir_display_name}/")

        file_indent = "    " * (level + 1)
        files.sort()
        for filename in files:
            print(f"{file_indent}{filename}")
            
    print("==========================================")


def log_file_contents_and_paths(start_abs_path, game_dir_actual_basename, exclude_abs_paths):
    """
    Logs the relative path and content of each file within start_abs_path.
    (Prints to current sys.stdout, which will be captured)
    """
    print(f"\nFile Paths and Contents in: {game_dir_actual_basename}")
    print("==========================================")
    
    for root, dirs, files in os.walk(start_abs_path, topdown=True):
        dirs[:] = [d for d in dirs if os.path.join(root, d) not in exclude_abs_paths]
        files.sort()
        
        for filename in files:
            file_abs_path = os.path.join(root, filename)
            relative_path = os.path.relpath(file_abs_path, start_abs_path)
            
            print(f"\n--- File: {relative_path} ---")
            try:
                with open(file_abs_path, 'r', encoding='utf-8', errors='strict') as f:
                    content = f.read()
                print(content)
            except UnicodeDecodeError:
                print("[Error: Could not decode file as UTF-8. It might be a binary file or use a different encoding.]")
            except IOError as e:
                print(f"[Error reading file: {e}]")
            except Exception as e:
                print(f"[An unexpected error occurred while reading file '{relative_path}': {e}]")
    print("==========================================")

def copy_to_clipboard(text_to_copy):
    """
    Attempts to copy the given text to the system clipboard.
    Returns a tuple (success_boolean, message_string).
    """
    os_name = platform.system()
    try:
        if os_name == "Windows":
            # Using Popen to pipe to clip.exe's stdin
            process = subprocess.Popen('clip', stdin=subprocess.PIPE, text=True, encoding='utf-8', errors='replace')
            process.communicate(input=text_to_copy)
            return True, "Output copied to clipboard (Windows)."
        elif os_name == "Darwin": # macOS
            process = subprocess.Popen('pbcopy', stdin=subprocess.PIPE, text=True, encoding='utf-8', errors='replace')
            process.communicate(input=text_to_copy)
            return True, "Output copied to clipboard (macOS)."
        elif os_name == "Linux":
            try: # Try xclip
                process = subprocess.Popen(['xclip', '-selection', 'clipboard'], stdin=subprocess.PIPE, text=True, encoding='utf-8', errors='replace')
                process.communicate(input=text_to_copy)
                return True, "Output copied to clipboard using xclip (Linux)."
            except FileNotFoundError: # xclip not found, try xsel
                try:
                    process = subprocess.Popen(['xsel', '--clipboard', '--input'], stdin=subprocess.PIPE, text=True, encoding='utf-8', errors='replace')
                    process.communicate(input=text_to_copy)
                    return True, "Output copied to clipboard using xsel (Linux)."
                except FileNotFoundError:
                    return False, "Could not copy to clipboard: 'xclip' or 'xsel' not found on this Linux system. Please install one of them (e.g., 'sudo apt install xclip')."
        else:
            return False, f"Clipboard operations not supported for OS: {os_name}."
    except subprocess.SubprocessError as e:
        return False, f"Error during clipboard operation: {e}"
    except Exception as e:
        return False, f"An unexpected error occurred during clipboard operation: {e}"


if __name__ == "__main__":
    # --- Path Setup ---
    current_working_dir = os.getcwd()
    game_dir_abs_path = os.path.join(current_working_dir, GAME_DIR_BASENAME)
    game_dir_abs_path = os.path.abspath(game_dir_abs_path) 

    if not os.path.isdir(game_dir_abs_path):
        # Print directly to stderr if basic setup fails
        print(f"Error: Directory '{GAME_DIR_BASENAME}' not found at expected path '{game_dir_abs_path}'.", file=sys.stderr)
        sys.exit(1)

    excluded_abs_paths = [os.path.join(game_dir_abs_path, d) for d in EXCLUDE_DIR_BASENAMES]

    # --- Capture Output ---
    original_stdout = sys.stdout
    captured_output_buffer = io.StringIO()
    sys.stdout = captured_output_buffer

    # --- Execute Logging Functions (output goes to buffer) ---
    log_directory_structure(game_dir_abs_path, GAME_DIR_BASENAME, excluded_abs_paths)
    log_file_contents_and_paths(game_dir_abs_path, GAME_DIR_BASENAME, excluded_abs_paths)
    print("\nScript finished (pre-clipboard).") # This also goes to buffer

    # --- Restore stdout & Get Captured Content ---
    sys.stdout = original_stdout
    full_output_string = captured_output_buffer.getvalue()
    captured_output_buffer.close()

    # --- Print Captured Output to Console ---
    print(full_output_string) # Now this prints to the actual console

    # --- Copy to Clipboard & Notify ---
    success, message = copy_to_clipboard(full_output_string)
    print(f"\n--- Clipboard Status ---")
    print(message)
    print(f"------------------------")

    if not success and platform.system() == "Linux" and "not found" in message:
        print("Tip: On Linux, you might need to install a clipboard utility like 'xclip' or 'xsel'.")
        print("For example: 'sudo apt install xclip' or 'sudo yum install xclip'")