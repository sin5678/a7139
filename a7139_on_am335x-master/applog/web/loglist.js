// JavaScript Document
function ret_check(ret)
{
    if (!ret || ret.indexOf("<html>") == -1) {
        return true;
    } else {
        return false;
    }
}

function applog_load(logtype,msg){
    ajaxdata = "cmd=refresh&logtype="+logtype;
    $.ajax({
        type:"GET",
        cache:false,
        async:true,
        url:"/cgi-bin/applog.cgi",
        timeout: 10000, //10秒
        data:ajaxdata,
        success:function(ret) {
            if (ret_check(ret)) {
                $("#area_"+logtype).val(ret);
                if (msg) {
                    $("#debug_refresh_clear_result").html("操作成功");
                    $("#debug_refresh_clear_result").removeClass().addClass("result_success_txt");
                }
            } else {
                $("#debug_refresh_clear_result").html("操作失败");
                $("#debug_refresh_clear_result").removeClass().addClass("result_fail_txt");
            }
        },
        error:function(ret) {
            if (msg) {
                $("#debug_refresh_clear_result").html("操作失败");
                $("#debug_refresh_clear_result").removeClass().addClass("result_fail_txt");
            }
        },
    });
    return true;
}

function applog_getconf(){
    ajaxdata = "cmd=getconf";
    $.ajax({
        type:"GET",
        cache:false,
        async:false,
        url:"/cgi-bin/applog.cgi",
        timeout: 10000, //10秒
        data:ajaxdata,
        success:function(ret){
            var items=ret.split("\n");
            $("input[name=debug_local_remote][value=local]").click();
            for (var i=0; i<items.length; i++) {
                //alert(items[i]);
                var pos = items[i].indexOf("=");
                if (pos == -1) continue;
                var item=items[i].substring(0,pos);
                var vale=items[i].substring(pos+1);
                //if ($.trim(item).length==0||$.trim(vale).length==0) continue;
                if (item.indexOf("debug_level_") == 0) {
                    dl_select = "<div>";
                    dl_select = dl_select + "<span class='fixwidth'>"+item+"</span>";
                    dl_select = dl_select + "<select name=\""+item+"\">";
                    dl_select = dl_select + "<option value='-1' selected>LOG_DEFAULT</option><option value='0'>LOG_EMERG</option><option value='1'>LOG_ALERT</option><option value='2'>LOG_CRIT</option><option value='3'>LOG_ERR</option><option value='4'>LOG_WARNING</option><option value='5'>LOG_NOTICE</option><option value='6'>LOG_INFO</option><option value='7'>LOG_DEBUG</option>";
                    dl_select = dl_select + "</select></div>";
                    $("#debug_applog").append(dl_select);
                } else if ($("#"+item).attr("type")=="checkbox") {
                    if (vale=="true"||vale==1) {
                        $("#"+item).attr("checked", vale);
                    } else {
                        $("#"+item).removeAttr("checked");
                    }
                } else if ($("input[name="+item+"]").attr("type")=="radio") {
                    //alert(item+","+vale);
                    $("input[name="+item+"][value="+vale+"]").click();
                } else {
                    $("#"+item).val(vale);
                }
            }
        },
        error:function(ret){
            alert("获取日志配置失败!");
        }
    });
    return true;
}

function applog_submit(cmd){
    var ajaxdata="";
    if (cmd == "set") {
        var debug_local_remote = $("input:checked[name=debug_local_remote]").val();
        var debug_local_pts = $("#debug_local_pts").attr("checked")?"1":"0";
        var debug_remote_ip = $("#debug_remote_ip").val();  /* todo check ip valid */
        var debug_applogs_level = $("#debug_applog select");
        ajaxdata += "cmd=" + cmd;
        ajaxdata += "&debug_local_remote=" + debug_local_remote;
        ajaxdata += "&debug_local_pts=" + debug_local_pts;
        ajaxdata += "&debug_remote_ip=" + debug_remote_ip;
        for (var i=0; i<debug_applogs_level.length; i++) {
            ajaxdata += "&"+debug_applogs_level[i].name+"="+debug_applogs_level[i].value;
        }
        //alert(ajaxdata);
        $("#resultdiv").html('<img src="pub/images/onLoad.gif"/> &nbsp; 配置中，请稍后...');
        $("input#log_save_btn").attr("disabled", "true");
        $.ajax({
            type:"POST",
            cache:false,
            url:"/cgi-bin/applog.cgi",
            timeout: 10000, //10秒
            data:ajaxdata,
            success:function(ret){
                if (ret_check(ret)) {
                    $("#resultdiv").html("配置成功!\n"+ret).wrapInner("<pre>");
                    $("#resultdiv").removeClass().addClass("result_success_txt");
                    $("input#log_save_btn").removeAttr("disabled");
                } else {
                    $("#resultdiv").html("配置失败!");
                    $("#resultdiv").removeClass().addClass("result_fail_txt");
                    $("input#log_save_btn").removeAttr("disabled");
                }
            },
            error:function(ret){
                $("#resultdiv").html("配置失败!\n"+ret).wrapInner("<pre>");
                $("#resultdiv").removeClass().addClass("result_fail_txt");
                $("input#log_save_btn").removeAttr("disabled");
            },
        });
    } else if (cmd == "clear") {
        ajaxdata+="cmd="+cmd;
        logtype=$("#fadetab li.current").get(0).id.replace(/_li/, "");
        ajaxdata+="&logtype="+logtype;
        $("#debug_refresh_clear_result").removeClass();
        $("#debug_refresh_clear_result").html('<img src="pub/images/onLoad.gif"/> &nbsp; 刷新中，请稍后...');
        $.ajax({
            type:"POST",
            cache:false,
            url:"/cgi-bin/applog.cgi",
            timeout: 10000, //10秒
            data:ajaxdata,
            success:function(ret){
                applog_load(logtype, 1);
            },
            error:function(ret){
                alert("清除日志失败!");
            },
        });
    } else if (cmd == "refresh") {
        $("#debug_refresh_clear_result").removeClass();
        $("#debug_refresh_clear_result").html('<img src="pub/images/onLoad.gif"/> &nbsp; 刷新中，请稍后...');
        applog_load("user_log", 1);
        applog_load("sys_log", 1);
        applog_load("debug_log", 1);
    }
    return false;
}
