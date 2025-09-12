import Html from "../../Libs/html.js";
import Ws from "../../Libs/wm.cherrytree.js";

let myWindow;

const pkg = {
  name: "Example App",
  type: "app",
  privs: 0,
  start: async function (Root) {
    // Testing
    console.log("Hello from example app", Root);

    // Create a window
    myWindow = new Libs.Window({
      title: "Example app",
    });

    // Get the window body
    const wrapper = myWindow.window.querySelector(".win-content");

    // Add content to the window
    new Html("h1").text("Example App").appendTo(wrapper);
    new Html("p").text("This is the example app").appendTo(wrapper);
  },
  end: async function () {
    // Close the window when the process is exited
    myWindow.close();
  },
};

export default pkg;