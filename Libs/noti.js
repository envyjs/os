class Notification {
    constructor() {
        this.container = document.getElementById('userland');
        if (!this.container) {
            this.container = document.createElement('div');
            this.container.id = 'userland';
            document.body.appendChild(this.container);
        }
    }

    // Method to create a notification
    create(message, type = 'info', duration = 3000) {
        const notification = document.createElement('div');
        notification.classList.add('notification', type);

        // Set the message
        notification.innerHTML = `
            <span>${message}</span>
            <span class="close-btn">&times;</span>
        `;

        // Append notification to the container
        this.container.appendChild(notification);

        // Show the notification (with animation)
        setTimeout(() => {
            notification.classList.add('show');
        }, 100);

        // Automatically remove after duration
        if (duration > 0) {
            setTimeout(() => {
                this.remove(notification);
            }, duration);
        }

        // Manual close button
        notification.querySelector('.close-btn').addEventListener('click', () => {
            this.remove(notification);
        });
    }

    // Method to remove a notification
    remove(notification) {
        notification.classList.remove('show');
        setTimeout(() => {
            if (notification && notification.parentNode) {
                this.container.removeChild(notification);
            }
        }, 300); // Match this with CSS transition duration
    }
}

// Create a new instance of the notification library
const notifier = new Notification();
console.log("[NOTI] Notification dependency loaded")  