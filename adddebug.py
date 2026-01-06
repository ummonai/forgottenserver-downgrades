import re

def inject_debug_to_members(file_path):
    # Regex Breakdown:
    # (\w+::\w+) -> Matches ClassName::MethodName
    # \(.*\)     -> Matches arguments
    # \s*\{      -> Matches optional whitespace and the opening brace
    member_func_pattern = r'(\w+::\w+\(.*\)\s*\{)'
    
    # Replacement adds the print statement immediately after the opening brace
    # __PRETTY_FUNCTION__ is great because it includes the Class Name and Args
    replacement = r'\1\n    std::cout << "[DEBUG] " << __PRETTY_FUNCTION__ << std::endl;'

    with open(file_path, 'r') as f:
        content = f.read()

    new_content = re.sub(member_func_pattern, replacement, content)

    with open(file_path, 'w') as f:
        f.write(new_content)
    
    print(f"Successfully processed {file_path}")

# Run the script
inject_debug_to_members('/mnt/shared/tibiadev/forgottenserver-downgrades/src/protocollogin.cpp')

