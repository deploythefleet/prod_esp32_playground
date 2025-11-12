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

# Function to set ESP32 target interactively
setchip() {
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
    
    while true; do
        echo "┌─────────────────────────────────────────────────────┐"
        echo "│                ESP32 Target Selection               │"
        echo "├─────────────────────────────────────────────────────┤"
        echo "│  1) esp32        │  6) esp32c6                      │"
        echo "│  2) esp32s2      │  7) esp32h2                      │"
        echo "│  3) esp32c3      │  8) esp32p4                      │"
        echo "│  4) esp32s3      │  9) esp32c5                      │"
        echo "│  5) esp32c2      │ 10) esp32c61                     │"
        echo "└─────────────────────────────────────────────────────┘"
        echo
        read -p "Select target (1-${#targets[@]}) or 'q' to quit: " choice
        
        # Check if user wants to quit
        if [[ "$choice" == "q" ]] || [[ "$choice" == "Q" ]]; then
            echo "Selection cancelled."
            return 0
        fi
        
        # Validate input
        if [[ ! "$choice" =~ ^[0-9]+$ ]] || [ "$choice" -lt 1 ] || [ "$choice" -gt "${#targets[@]}" ]; then
            echo "❌ Error: Invalid selection. Please choose a number between 1 and ${#targets[@]}, or 'q' to quit."
            echo
            continue
        fi
        
        # Set the target (array is 0-indexed, user input is 1-indexed)
        local selected_target="${targets[$((choice-1))]}"
        export IDF_TARGET="$selected_target"
        
        echo "✅ IDF_TARGET set to: $IDF_TARGET"
        echo ""
        
        # Update the prompt to reflect the new target
        update_prompt
        break
    done
    
    # Update the prompt to reflect the new target
    update_prompt
}