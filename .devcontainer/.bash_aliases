# Store the original PS1 before any modifications
ORIGINAL_PS1="$PS1"

# Function to update prompt with current IDF_TARGET
update_prompt() {
    # Create a custom PS1 without hostname, keeping username and working directory
    CUSTOM_PS1="\[\e]0;\u: \w\a\]${debian_chroot:+($debian_chroot)}\[\033[01;32m\]\u\[\033[00m\]:\[\033[01;34m\]\W\[\033[00m\]\$ "
    PS1="(${IDF_TARGET:-missing-target})$CUSTOM_PS1"
}

# Set PROMPT_COMMAND to update prompt before each command
PROMPT_COMMAND="update_prompt"

workspace_root="/workspaces/prod_esp32_playground"


# User commands

# Function to set ESP32 target interactively
setchip() {
    local selected_target="$1"
    local targets=(
        "esp32"
        "esp32s2"
        "esp32c3"
        "esp32s3"
        "esp32c2"
        "esp32c6"
        "esp32h2"
        "esp32p4"
        "esp32c5"
        "esp32c61"
    )

    if [ -n "$selected_target" ]; then
        if [[ ! " ${targets[@]} " =~ " ${selected_target} " ]]; then
            _log "❌ Invalid target: $selected_target. Valid targets are: ${targets[*]}"
            return 1
        fi
    else
        while true; do
            _print_table "     ESP32 Target Selection" targets 2
            read -p "Select target (1-${#targets[@]}) or 'q' to quit: " choice
            # Check if user wants to quit
            if [[ "$choice" == "q" ]] || [[ "$choice" == "Q" ]]; then
                echo "Selection cancelled."
                return 0
            fi
            # Validate input
            if [[ ! "$choice" =~ ^[0-9]+$ ]] || [ "$choice" -lt 1 ] || [ "$choice" -gt "${#targets[@]}" ]; then
                _log "❌ Invalid selection. Please choose a number between 1 and ${#targets[@]}, or 'q' to quit."
                _log ""
                continue
            fi
            selected_target="${targets[$((choice-1))]}"
            break
        done
    fi
    # Set the target (array is 0-indexed, user input is 1-indexed)
    export IDF_TARGET="$selected_target"

    # Determine GDB path based on target
    _update_vscode_settings "$selected_target"

    _log "✅ IDF_TARGET set to: $IDF_TARGET"

    # Update the prompt to reflect the new target
    update_prompt
}

_build_preparation() {
    local build_on_change="$1"

    # 1. Check if CMakeLists.txt exists in current directory
    if [ ! -f "CMakeLists.txt" ]; then
        _log "❌ No CMakeLists.txt found in current directory"
        return 1
    fi
    # 2. Update VS Code settings
    local target="${IDF_TARGET:-esp32s3}"
    _update_vscode_settings "$target"
    if [ $? -ne 0 ]; then
        return 1
    fi
    # 3. Check if current build target matches selected target, if not, configure and build the project
    local current_target=$(_get_config_parameter "CONFIG_IDF_TARGET" "build/sdkconfig")
    if [ -z "$current_target" ]; then
        current_target=$(_get_config_parameter "CONFIG_IDF_TARGET" "sdkconfig")
    fi
    if [ "$current_target" != "$target" ]; then
        _log "🔧 Configuring and building the project for debugging with $target..."
        idf.py set-target "$target" > /dev/null 2>&1

        export CCACHE_BASEDIR=$PWD
        if [ -n "$build_on_change" ]; then
            idf.py build > /dev/null 2>&1
        fi
    fi
}

# Function to set active debug project
debugthis() {
    _build_preparation "build_on_change"
}

# Function to create a new ESP32 project from minimal_build template
np() {
    local project_name="$1"

    local examples_dir="$workspace_root/examples"
    local template_dir="$examples_dir/minimal_build"

    if [[ -z "${project_name// }" ]]; then
        read -p "Enter new project name: " project_name

        # Validate project name is not empty
        if [[ -z "${project_name// }" ]]; then
            _log "❌ Project name cannot be empty"
            return 1
        fi
    fi
    project_name="${project_name// }"
    
    # Create new project directory path
    local new_project_dir="$examples_dir/$project_name"
    
    # Check if directory already exists
    if [ -d "$new_project_dir" ]; then
        _log "❌ Project directory already exists: $new_project_dir"
        return 1
    fi

    # Copy minimal_build to new project directory
    _log "📁 Creating new project '$project_name' from minimal_build template..."
    cp -r "$template_dir" "$new_project_dir"
    
    if [ $? -ne 0 ]; then
        _log " ❌ Failed to copy template"
        return 1
    fi

    # Remove the build folder if it exists
    if [ -d "$new_project_dir/build" ]; then
        rm -rf "$new_project_dir/build"
    fi

    # Update README.md with just the project name as H1
    echo "# $project_name" > "$new_project_dir/README.md"
    
    # Update CMakeLists.txt to use new project name
    sed -i "s/project(minimal_build)/project($project_name)/" "$new_project_dir/CMakeLists.txt"
    
    # Update main.cpp to reflect new project name
    sed -i "s/Hello, Minimal Build!/Hello, $project_name!/" "$new_project_dir/main/main.cpp"
    
    _log "✅ Project '$project_name' created successfully at: $new_project_dir"

    # Change to the new project directory
    cd "$new_project_dir"
}


# ccache configuration
export CCACHE_NOHASHDIR=true                # Avoid hashing the full path of source files, which can cause cache misses in certain build systems
export CMAKE_C_COMPILER_LAUNCHER=ccache     # Use ccache for C compilation
export CMAKE_CXX_COMPILER_LAUNCHER=ccache   # Use ccache for C++ compilation
ccache -M 10G > /dev/null 2>&1              # Set maximum cache size to 10GB, does not hurt to set it every time on terminal init

build() {
    _build_preparation
    idf.py build
}

# Wrapper for sudo that preserves user environment PATH, which is necessary for using ESP-IDF tools with sudo
psudo() {
    sudo env PATH="$PATH" $@
}


# Helper functions

_log() {
    echo "$1" >&2
}

# Function to print a table with dynamic column widths
# Arguments:
# 1. Table title (string)
# 2. Array of table rows (passed by name using nameref)
# 3. Number of columns (integer)
_print_table() {
    local table_title=$1
    local -n table=$2
    local columns_count=$3

    local table_row_count=${#table[@]}
    # Calculate column widths
    local max_width=0
    for ((i=0; i<$table_row_count; i++)); do
        local length=${#table[i]}
        if [ $length -gt $max_width ]; then
            max_width=$length
        fi
    done
    local column_width=($((max_width + 2))) # Add padding
    local data_width=$((column_width + 4))  # Account for numbering and extra spaces
    local padded_column_width=$((data_width + 2)) # Add padding for the table borders
    local table_width=$((padded_column_width * $columns_count + $columns_count))
    # Print table header
    _log "┌$(printf '─%.0s' $(seq 1 $table_width))┐"
    _log "│$(printf '%-*s' $table_width "$table_title")│"
    _log "├$(printf '─%.0s' $(seq 1 $table_width))┤"
    # Print table rows
    # `+columns_count-1` is to get ceiling of division for part_size
    local part_size=$((($table_row_count+$columns_count-1)/$columns_count))
    for((i=0;i<$part_size;i++)); do
        for((j=0;j<$columns_count;j++)); do
            local index=$((i+j*part_size))
            if [ $index -lt ${#table[@]} ]; then
                printf "| %2d) %-${column_width}s "  "$((index+1))" "${table[$index]}"
            else
                printf "| %-${data_width}s " " "
            fi
        done
        printf " |\n";
    done
    # Print table footer
    _log "└$(printf '─%.0s' $(seq 1 $table_width))┘"
}

_get_config_parameter() {
    local key="$1"
    local config_file="$2"

    echo $(sed -n -e "s/^$key=\"*\([a-z0-9_-]\+\)\"*$/\1/p" $config_file 2>/dev/null)
}

_add_or_replace_json_value() {
    local key="$1"
    local value="$2"
    local settings_file="$3"

    # Create settings file if it doesn't exist
    if [ ! -f "$settings_file" ]; then
        mkdir -p "$(dirname "$settings_file")"
        echo "{\n}" > "$settings_file"
    fi

    # Add or replace the key-value pair in the settings file
    if grep -q "\"$key\"" "$settings_file"; then
        sed -i "s|\"$key\": \".*\"|\"$key\": \"$value\"|" "$settings_file"
    else
        sed -i "s|{|\{\n    \"$key\": \"$value\",|" "$settings_file"
    fi
}

_get_debugger() {
    local target="$1"
    if [ "$target" = "esp32c3" ]; then
        gdb_path=$(which riscv32-esp-elf-gdb)
    else
        gdb_path=$(which xtensa-esp32s3-elf-gdb)
    fi
    if [ -z "$gdb_path" ]; then
        _log "❌ Could not find GDB for target $target. Please ensure the appropriate toolchain is installed."
        return 1
    fi
    echo "$gdb_path"
}

_update_vscode_settings() {
    local target="$1"

    local settings_file="$workspace_root/.vscode/settings.json"
    local relative_path="${PWD#$workspace_root/}"
    local project_name=$(sed -n -e "s/^project(\([a-z0-9_-]\+\))/\1/p" CMakeLists.txt 2>/dev/null | head -1)
    if [ -z "$project_name" ]; then
        _log "❌ Could not find project() in CMakeLists.txt"
        project_name="unknown-project"
    else
        _log "✅ Active project is: $project_name"
    fi

    _add_or_replace_json_value "idf.target" "${target}" "$settings_file"
    _add_or_replace_json_value "esp32.activeProject" "$relative_path" "$settings_file"
    _add_or_replace_json_value "esp32.activeProjectName" "$project_name" "$settings_file"
    _add_or_replace_json_value "cmake.sourceDirectory" "$PWD/main" "$settings_file"

    local gdb_path=$(_get_debugger "$target")
    if [ $? -ne 0 ]; then
        return 1
    fi
    _add_or_replace_json_value "esp32.gdbPath" "$gdb_path" "$settings_file"
}
