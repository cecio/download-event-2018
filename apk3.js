setImmediate(function() { //prevent timeout
  console.log("[*] Starting script");

  Java.perform(function () {
    var cls_launcher_activity = Java.use("de.fraunhofer.sit.premiumapp.LauncherActivity");
    var cls_main_activity = Java.use("de.fraunhofer.sit.premiumapp.MainActivity");

    /**********************************************************************/
    function strToBytes(str) {
      var chr_temp;
      var str_temp;
      var str_return = [];
      for (var x = 0; x < str.length; x++ ) {
        str_temp = [];
        chr_temp = str.charCodeAt(x);
        do {
          str_temp.push( chr_temp & 0xFF );
          chr_temp = chr_temp >> 8;
        } while ( chr_temp );
        str_return = str_return.concat( str_temp.reverse() );
      }
      return str_return;
    }
    function byteToStr(arr) {
      var str_temp = "";

      for(i = 0; i < arr.length; i++)
      {
        str_temp += String.fromCharCode(arr[i]);
      }
      return str_temp;
    }
    /**********************************************************************/

    cls_launcher_activity.verifyClick.implementation = function () {
      console.log("[*] verifyClick got called!\n");
      console.log("[*] Read MAC: " + this.getMac());
      strMAC = strToBytes(this.getMac());
      strLIC = strToBytes("LICENSEKEYOK");
      strKEY = byteToStr(cls_main_activity.xor(strMAC,strLIC));
      console.log("[*] Computed KEY: " + strKEY);

      editor = this.getApplicationContext().getSharedPreferences("preferences", 0).edit();
      editor.putString("KEY", strKEY);
      editor.commit();
      console.log("[*] Exiting verifyCLick\n");
    };
  });
});
