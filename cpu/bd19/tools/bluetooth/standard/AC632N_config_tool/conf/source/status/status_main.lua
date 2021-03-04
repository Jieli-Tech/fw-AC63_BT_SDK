status_view_tabs = {};
status_output_text_tabs = {};
status_output_bin_tabs = {};


if enable_moudles["status"] == false then
    return;
else

require("status.status_v1");

status_view = {"状态配置",
    cfg:vBox(
        status_view_tabs
    )
};

end
