#!/bin/bash

# initialises source for liveCD creation

show_menu() {
    echo""
    echo "This script MUST be run from /Tools/, not root of source, or else it will fuck up a LOT of your files. Are you sure it is running from /Tools/? (yn)"
    echo""
}

# Main loop for menu selection
while true; do
    show_menu
    read -r choice

    case $choice in
        y)
            cd ..
            cd Assets
            rm debian.ext2
            cd ..
            rm -rf Platforms
            rm -rf Submodules
            rm -rf Kernel
            rm -rf Docs
            rm -rf .github
            rm -rf .vscode
            rm -rf Tools
            break 
            ;;

        n)
            echo "Exiting..."
            break
            ;;
        *)
            echo "Invalid choice. Returning to menu."
            ;;
    esac
done