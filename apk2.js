setImmediate(function() { //prevent timeout
  console.log("[*] Starting script");

  Java.perform(function () {
    var cls_launcher_activity = Java.use("de.fraunhofer.sit.premiumapp.LauncherActivity");

      cls_launcher_activity.verifyClick.implementation = function () {
      console.log("[*] verifyClick got called!\n");
      console.log("[*] Read MAC: " + this.getMac());

      editor = this.getApplicationContext().getSharedPreferences("preferences", 0).edit();
      editor.putString("KEY", "Trythisone");
      editor.commit();
      console.log("[*] Exiting verifyCLick\n");
    };
  });
});
