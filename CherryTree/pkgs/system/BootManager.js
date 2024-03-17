import Html from "../../libs/html.js";

let wrapper;

const pkg = {
  name: "Boot Manager",
  type: "app",
  privs: 1,
  start: async function (Root) {
    console.log("Hello from BM app", Root);
    const loadingScreen = document.querySelector("#loading");
    if (loadingScreen) loadingScreen.remove();

    wrapper = new Html("p").text("Welcome").appendTo("body");

    await Root.Core.pkg.run("ui:Desktop", [], true);
    await Root.Core.pkg.run("apps:Example", [], true);
  },
  end: async function () {
    wrapper.cleanup();
  },
};

export default pkg;