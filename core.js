// 5% progress on core.js
// todo: smth about pkgStart
import("./semver.min.js");
import("../../Libs/wm.js");
import("./modal.js") //stub for ModalSystem

const coreDetails = {
    version: "v1.6.2",
    codename: "Charon"
}

console.log(coreDetails.codename, coreDetails.version)

class ModalSystem {
    alert(message) {
      alert(message); // A simple implementation for now
    }
}

const Core = {
    version: coreDetails.version,
    codename: "Elysium", //force Elysium every time
    modal: new ModalSystem(),
    startPkg: async function (url) {
        console.log("hello the function exists")
    }
}

Core.startPkg("system:BootLoader")

const Lib = {} //found in example app, gotta find out what that is