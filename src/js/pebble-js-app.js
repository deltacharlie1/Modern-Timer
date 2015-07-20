var vibes_key = 0;
var revert_key = 1;
var screen_type_key = 2;
var screen_color_key = 3;
var flipscreen_key = 4;
var vibes_status;
var revert_status;
var screen_type = "sb";
var watch;
var screen_color;
var flipscreen = "no";

Pebble.addEventListener("ready", function(e) {
  console.log("Hello world! - Sent from your javascript application.");
});
Pebble.addEventListener('showConfiguration', function(e) {
        console.log("Into configuration.");
  if (Pebble.getActiveWatchInfo) {
    watch = Pebble.getActiveWatchInfo();
  }
    
  vibes_status = localStorage.getItem(vibes_key);
  if (vibes_status == null) {
    vibes_status = "on";
  }
  revert_status = localStorage.getItem(revert_key);
  if (revert_status == null) {
    revert_status = "no";
  }
  screen_type = localStorage.getItem(screen_type_key);
  if (screen_type == null) {
    screen_type = "sb";
  }
  flipscreen = localStorage.getItem(flipscreen_key);
  if (flipscreen == null) {
    flipscreen = "no";
  }
  screen_color = localStorage.getItem(screen_color_key);
  if (screen_color == null) {
    screen_color = "rgb(0, 0, 0)";
  }

  Pebble.openURL('http://www.corunna.com/Pebble/index.html?' + vibes_status + "?" + revert_status + "?" + screen_type + "?" + watch.platform + "?" + encodeURIComponent(screen_color) + "?" + flipscreen);
});
Pebble.addEventListener('webviewclosed', function(e) {
        console.log("Into wbclosed.");
  var options = JSON.parse(e.response);
  if (options.vibes_on_on) {
    localStorage.setItem(vibes_key, "on");
    vibes_status = "on";
  }
  if (options.vibes_on_off) {
    localStorage.setItem(vibes_key, "off");
    vibes_status = "off";
  }
  if (options.revert_no) {
    localStorage.setItem(revert_key, "no");
    revert_status = "no";
  }
  if (options.revert_yes) {
    localStorage.setItem(revert_key, "yes");
    revert_status = "yes";
  }
  if (options.flipscreen_no) {
    localStorage.setItem(flipscreen_key, "no");
    flipscreen = "no";
  }
  if (options.flipscreen_yes) {
    localStorage.setItem(flipscreen_key, "yes");
    flipscreen = "yes";
  }
  if (options.screen_type_std_b) {
    localStorage.setItem(screen_type_key, "sb");
    screen_type = "sb";
  }
  if (options.screen_type_bold_b) {
    localStorage.setItem(screen_type_key, "bb");
    screen_type = "bb";
  }
  if (options.screen_type_std_w) {
    localStorage.setItem(screen_type_key, "sw");
    screen_type = "sw";
  }
  if (options.screen_type_bold_w) {
    localStorage.setItem(screen_type_key, "bw");
    screen_type = "bw";
  }
  if (Pebble.getActiveWatchInfo) {
    localStorage.setItem(screen_color_key, options.watchface_color);
  }
  var rgb = options.watchface_color.match(/^rgba?\((\d+),\s*(\d+),\s*(\d+)(?:,\s*(\d+))?\)$/);
  Pebble.sendAppMessage( { "VIBES_STATUS": vibes_status,
                           "REVERT_STATUS": revert_status,
                           "SCREEN_TYPE": screen_type,
                           "RGB_RED": rgb[1],
                           "RGB_GREEN": rgb[2],
                           "RGB_BLUE": rgb[3],
                           "FLIPSCREEN": flipscreen
                         },
      function(e) {
        console.log("Sending settings data ...");
      },
      function(e) {
        console.log("Settings feedback failed");
      });
});
