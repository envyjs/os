
////////////////////////
//  Cherry Tree Core  //
//  By kat21 and lap  //
////////////////////////
/*
     ///////////     @@@@@@@
      //        //            
        //        //   @@@@   @ @@  @   @ @   @
      //        //            @@  @  @ @   @ @
     ///////////     @@@@@@@  @   @   @     @
                                           @
*/
// Modified for use on Envy v8.0
import Html from "./libs/html.js";
import Ws from "./libs/windowSystem.js";

(async () => {
  let Security = {};
  let Libs = {
    Html,
    startPkg(pkgName, args) {
      return Core.pkg.run(pkgName, args, false);
    },
    Window: Ws.data.win,
  };
  let Processes = {
    list: [],

    get(id) {
      const i = Processes.list[id];
      if (i) {
        return {
          name: i.name,
          pid: i.pid,
          type: i.type,
        };
      }
    },
    getService(name) {
      const p = Processes.list
        .filter((p) => p.type === "svc")
        .find((p) => p.svcName === name);
      if (p !== undefined) return { data: p.data };
    },
  };
  let Core = {
    process: {
      findEmptyPid: function () {
        let r = Processes.list.findIndex((p) => p === null);
        return r !== -1 ? r : Processes.list.length;
      },

      cleanup: function (pid) {
        if (Processes.list[pid]) Processes.list[pid] = null;
        return;
      },
    },
    pkg: {
      startFromUrl: async function (
        url,
        Arguments,
        RunWithoutSecurity = false
      ) {
        const pkg = await import(url);
        const pkgData = pkg.default;
        let privs = false;

        // Validate package information
        if (!pkgData || typeof pkgData !== "object") return false;
        if (!pkgData.start || typeof pkgData.start !== "function") return false;
        if (!pkgData.end || typeof pkgData.end !== "function") return false;
        if (!pkgData.name || typeof pkgData.name !== "string") return false;
        if (!pkgData.type || typeof pkgData.type !== "string") return false;
        if (
          pkgData.svcName !== undefined &&
          typeof pkgData.svcName !== "string"
        )
          return false;
        if (pkgData.data !== undefined && typeof pkgData.data !== "object")
          return false;
        if (pkgData.privs === undefined || typeof pkgData.privs !== "number")
          return false;

        // Bypass security checks (For system apps)
        if (RunWithoutSecurity === false) {
          if (pkgData.privs === 1) {
            if (
              confirm(
                `The app ${pkgData.name} wants to start with privileges. Confirm or deny?`
              ) === true
            ) {
              privs = true;
            }
          }
        } else {
          privs = pkgData.privs === 1 ? true : false;
        }

        // Look into the package data
        const pkgHasData = pkgData.data !== undefined;
        let data = undefined;

        // Assign package type (usually "app")
        let pkgType = pkgData.type;
        let svcName = pkgData.svcName;

        if (pkgHasData) {
          data = pkgData.data;
          pkgType = "svc"; // Service means it does have accessible data
        }

        const pid = Core.process.findEmptyPid();

        Processes.list[pid] = {
          pid,
          name: pkgData.name,
          type: pkgType,
          privs: pkgData.privs,
          data,
          async end() {
            console.log("Attempting to end pkg:", pkgData.name);

            const result = await pkgData.end();

            // If the process doesn't want to end it can return false
            if (result !== false) Core.process.cleanup(pid);
          },
        };

        if (svcName !== undefined) Processes.list[pid].svcName = svcName;

        const Root = {
          Arguments,
          Libs,
          Core: privs === true ? Core : undefined,
          Processes: {
            list: privs === true ? Processes.list : undefined,
            get: Processes.get,
          },
        };

        pkg.default.start(Root);

        return Processes.list[pid];
      },

      run: async function (url, args, rwp) {
        const appCat = url.split(":");
        const cat = appCat[0];
        const nam = appCat[1];
        const securedCats = ["system", "ui"];
        return await this.startFromUrl(
          `./pkgs/${cat}/${nam}.js`,
          args,
          rwp !== true ? securedCats.includes(cat) : true
        );
      },
    },
  };

  // For debugging purposes
  window.Core = Core;
  window.Processes = Processes;
  window.Security = Security;
  window.Libs = Libs;

  Ws.init(Core);

  Core.pkg.run("system:BootManager", {
    time: performance.now(),
  });
})();
