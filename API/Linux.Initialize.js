      // The read-only disk image from Leaning Technologies' fast cloud backend
      const cloudDevice = await CheerpX.CloudDevice.create(
        "./Assets/debian.ext2"
      );
      // Read-write local storage for disk blocks, it is used both as a cache and as persisteny writable storage
      const idbDevice = await CheerpX.IDBDevice.create("block1");
      // A device to overlay the local changes to the disk with the remote read-only image
      const overlayDevice = await CheerpX.OverlayDevice.create(
        cloudDevice,
        idbDevice
      );
      // Direct acces to files in your HTTP server
      const webDevice = await CheerpX.WebDevice.create("");
      // Convenient access to JavaScript binary data and strings
      const dataDevice = await CheerpX.DataDevice.create();

      const cx = await CheerpX.Linux.create({
        mounts: [
          { type: "ext2", path: "/", dev: overlayDevice },
          { type: "dir", path: "/app", dev: webDevice },
          { type: "dir", path: "/data", dev: dataDevice },
          { type: "devs", path: "/dev" },
        ],
      });

      // Interact with a console
      cx.setConsole(document.getElementById("console"));

      // Run a full-featured shell in your browser.
      await cx.run("/bin/bash", ["--login"], {
        env: [
          "HOME=/home/user",
          "USER=user",
          "SHELL=/bin/bash",
          "EDITOR=vim",
          "LANG=en_US.UTF-8",
          "LC_ALL=C",
        ],
        cwd: "/home/user",
        uid: 1000,
        gid: 1000,
      });