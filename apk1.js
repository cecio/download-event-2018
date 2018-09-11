setImmediate(function() { //prevent timeout
  console.log("[*] Starting script");

  Java.perform(function () {
    var cls_launcher_activity = Java.use("de.fraunhofer.sit.premiumapp.LauncherActivity");

    cls_launcher_activity.verifyClick.implementation = function () {
      console.log("[*] verifyClick got called!\n");
    };
  });
});
