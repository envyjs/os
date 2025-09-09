#!/bin/bash

# Function to display the menu
show_menu() {
    echo""
    echo "Envy Kernel Build Menu"
    echo "======================"
    echo "Please select a build architecture:"
    echo "1. Build for i386"
    echo "2. Build for AMD64"
    echo""
    echo "Enter a number from the list: "
}

# Main loop for menu selection
while true; do
    show_menu
    read -r choice

    case $choice in
        1)
            echo "Building Envy Kernel for i386"
            break
            ;;
        2)
            echo "Building Envy Kernel for AMD64"
            break
            ;;
        *)
            echo "Invalid choice."
            ;;
    esac
done