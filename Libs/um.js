// Envy User Management
function hashPasswordWithSalt(password, salt) {
    return CryptoJS.SHA256(password + salt).toString();
}

function registerUser() {
    const username = document.getElementById("registerUsername").value;
    const password = document.getElementById("registerPassword").value;
    const messageEl = document.getElementById("registerMessage");

    if (!username || !password) {
        messageEl.textContent = "Please fill in all fields.";
        return;
    }

    const users = JSON.parse(localStorage.getItem("users")) || {};

    if (users[username]) {
        messageEl.textContent = "Username already taken.";
        return;
    }

    const salt = CryptoJS.lib.WordArray.random(16).toString();
    const hashedPassword = hashPasswordWithSalt(password, salt);

    users[username] = { hashedPassword, salt };
    localStorage.setItem("users", JSON.stringify(users));

    messageEl.textContent = "User registered successfully!";
    messageEl.style.color = "green";
}

function loginUser() {
    const username = document.getElementById("loginUsername").value;
    const password = document.getElementById("loginPassword").value;
    const messageEl = document.getElementById("loginMessage");

    if (!username || !password) {
        messageEl.textContent = "Please fill in all fields.";
        return;
    }

    const users = JSON.parse(localStorage.getItem("users")) || {};
    if (!users[username]) {
        messageEl.textContent = "User not found.";
        return;
    }

    const { hashedPassword, salt } = users[username];
    const hashedInputPassword = hashPasswordWithSalt(password, salt);

    if (hashedInputPassword === hashedPassword) {
        messageEl.textContent = "Login successful!";
        messageEl.style.color = "green";
    } else {
        messageEl.textContent = "Invalid credentials.";
    }
}

console.log("[UM] Multi-User Enviornment loaded")