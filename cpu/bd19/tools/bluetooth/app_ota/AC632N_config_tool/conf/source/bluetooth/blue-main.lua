bt_output_vbox_view = {};
bt_output_htext_tabs = {};
bt_output_ctext_tabs = {};
bt_output_bin_tabs = {};

if enable_moudles["bluetooth"] == false then
    return;
else

require("bluetooth.bluetooth_v1");

bluetooth_view = {"蓝牙配置", bt_output_vbox_view};

end


