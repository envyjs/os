import Html from "../../libs/html.js";

let wrapper;

const pkg = {
  name: "Example App",
  type: "app",
  privs: 0,
  start: async function (Root) {
    // Testing
    console.log("Hello from example app", Root);

    // Get the window body
    wrapper = new Html("div").class("desktop").appendTo("body");

    // Add content to the window
    new Html("h1").text("Desktop").appendTo(wrapper);

    const taskbar = new Html("div").class("taskbar").appendTo(wrapper);

    const taskbarDock = new Html("div").class("taskbar-dock").appendTo(taskbar);

    // apps.forEach((a) => {
    //   taskbarDock.append(new Html());
    // });
  },
  end: async function () {
    // Close the window when the process is exited
    wrapper.cleanup();
  },
};

export default pkg;