<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="pub/css/skin.css" rel="stylesheet" type="text/css" />
<link href="pub/css/tabso.css" rel="stylesheet" type="text/css" />
<link href="pub/css/validator.css" type="text/css" rel="stylesheet"/>
<script src="pub/js/jquery.min.js" type="text/javascript"></script>
<script type="text/javascript" src="pub/js/jquery.tabso.js"></script>
<script src="pub/formvalidator/formvalidator.min.js" type="text/javascript"></script>
<script src="pub/formvalidator/formvalidatorregex.js" type="text/javascript"></script>
<script src="pub/js/common.js" type="text/javascript"></script>
<script src="pub/js/user/loglist.js" type="text/javascript"></script>
<script type="text/javascript">
$(document).ready(function($){
    var username = "<% session('username'); %>";
    $("#fadetab").tabso({
        cntSelect:"#fadecon",
        tabEvent:"click",
        tabStyle:"fade"
    });
    $("#debug_local").click(function(){
		$("#debug_remote_ip").attr("disabled",true).unFormValidator(true); 
		$("#debug_remote_ip_tip").html("");
        $("#debug_local_pts").removeAttr("disabled");
    });
    $("#debug_remote").click(function(){
        $("#debug_local_pts").attr("disabled","true");
        $("#debug_remote_ip").removeAttr("disabled").unFormValidator(false);
    });
    $("textarea").click(function(){
        $("#debug_refresh_clear_result").html("");
    });
    $("#resultdiv").click(function(){
        $("#resultdiv").html("");
    });
    if(username != "developer") {
        $("#log_setting_head").hide();
        $("#log_setting_tab").hide();
        $("#sys_log_li").hide();
        $("#debug_log_li").hide();
        $("#log_develop_span").hide();
    }
	
	$.formValidator.initConfig({formID:"debug_config_form",theme:'default',
		onError:function(msg){
			alert(msg);
			$("#resultdiv").html("");
			return false;
		},
		onSuccess:function(){
			return applog_submit("set");
		}
	});
	$("#debug_remote_ip").formValidator({tipID:"debug_remote_ip_tip",onShow:"",onFocus:"格式例如:192.168.1.1",onCorrect:""});
    $("#debug_remote_ip").regexValidator({regExp:"ip4",dataType:"enum",onError:"服务器IP格式不正确"});
	
    applog_getconf();
	if($("#debug_local").attr("checked")) {
		$("#debug_remote_ip").unFormValidator(true); 
	}
    applog_load("user_log", 0);
    applog_load("sys_log", 0);
    applog_load("debug_log", 0);
});
</script>
<title>设备日志</title>
</head>
<body onLoad="checkresult('resultdiv');">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td width="17" valign="top" background="pub/images/mail_leftbg.gif"><img src="pub/images/left-top-right.gif" width="17" height="29" /></td>
    <td valign="top" background="pub/images/content-bg.gif"><table width="100%" border="0" cellpadding="0" cellspacing="0" class="left_topbg" id="table2">
        <tr>
          <td height="31"><div class="titlebt">设备日志</div></td>
        </tr>
      </table></td>
    <td width="16" valign="top" background="pub/images/mail_rightbg.gif"><img src="pub/images/nav-right-bg.gif" width="16" height="29" /></td>
  </tr>
  <tr>
    <td valign="middle" background="pub/images/mail_leftbg.gif">&nbsp;</td>
    <td valign="top" align="center" height="100%" width="100%" bgcolor="#F7F8F9"><!-- 内容填充区begin -->
      
      <form action="#" method="post" id="debug_config_form" name="debug_config_form">
        <table width="95%" border="0" align="center" cellpadding="0" cellspacing="0">
          <tr>
            <td class="left_txt">当前位置：设备日志</td>
          </tr>
          <tr>
            <td height="20"><table width="100%" height="1" border="0" cellpadding="0" cellspacing="0" bgcolor="#CCCCCC">
                <tr>
                  <td></td>
                </tr>
              </table></td>
          </tr>
          <tr>
            <td><table width="100%" height="55" border="0" cellpadding="0" cellspacing="0">
                <tr>
                  <td width="10%" height="55"><img src="pub/images/mime.gif" width="54" height="55"></td>
                  <td width="90%"><span class="left_txt2">在这里，您可以</span> <span class="left_txt3">查看用户日志</span> <span id="log_develop_span" class="left_txt3">、系统日志、调试日志、启用或禁用模块的调试日志</span> <span class="left_txt2">！</span></td>
                </tr>
              </table></td>
          </tr>
          <tr>
            <td>&nbsp;</td>
          </tr>
          <tr id="log_setting_head">
            <td><table class="nowtable tbl" width="100%" border="0" cellpadding="0" cellspacing="0">
                <tr>
                  <td class="left_bt2">日志选项</td>
                </tr>
              </table></td>
          </tr>
          <tr id="log_setting_tab">
            <td><table width="100%" border="0" cellpadding="0" cellspacing="0">
                <tr class="t1">
                  <td width="20%" align="right" class="left_txt">日志模式：</td>
                  <td width="2%">&nbsp;</td>
                  <td width="30%" align="left" height="30" valign="middle" class="left_txt">
                      <label><span class="tag"> <input type="radio" id="debug_local" name="debug_local_remote" value="local" checked>本地日志</input></span></label>
                      <label><span class="tag"> <input type="checkbox" id="debug_local_pts" name="debug_local_pts"/>远程终端输出</span></label>
                  </td>
                  <td>&nbsp;</td>
                </tr>
                <tr>
                  <td align="right" class="left_txt" width="20%">&nbsp;</td>
                  <td width="2%">&nbsp;</td>
                  <td align="left" height="30" valign="middle" class="left_txt">
                    <label><span class="tag"> <input type="radio" id="debug_remote" name="debug_local_remote" value="remote">远程日志</input></span></label>
                    <label><span class="tag">日志服务器IP：<input type="text" id="debug_remote_ip" name="debug_remote_ip"></input></span></label>
                  </td>
                  <td><div id="debug_remote_ip_tip">&nbsp;</div></td>
                </tr>
                <tr class="t1">
                  <td align="right" class="left_txt" width="20%">调试日志：</td>
                  <td width="2%">&nbsp;</td>
                  <td id="debug_applog" align="left" height="30" valign="middle" class="left_txt"> </td>
                  <td>&nbsp;</td>
                </tr>
                <tr>
                  <td>&nbsp;</td>
                  <td>&nbsp;</td>
                  <td align="left"><div id="resultdiv"> &nbsp; </div></td>
                  <td>&nbsp;</td>
                </tr>
                <tr>
                  <td align="right" class="left_txt" width="20%">&nbsp;</td>
                  <td width="2%">&nbsp;</td>
                  <td align="left"><input id="log_save_btn" type="submit" value="配置"></td>
                  <td>&nbsp;</td>
                </tr>
                <tr>
                  <td>&nbsp;</td>
                </tr>
              </table></td>
          </tr>
          <tr>
            <td><div class="tabbox">
                <ul class="tabbtn" id="fadetab">
                  <li id="user_log_li" class="current"><a href="javascript:void(0);">用户日志</a></li>
                  <li id="sys_log_li"><a href="javascript:void(0);">系统日志</a></li>
                  <li id="debug_log_li"><a href="javascript:void(0);">调试日志</a></li>
                </ul>
                <!--tabbtn end-->
                <div class="tabcon" id="fadecon">
                  <div class="sublist">
                    <table width="100%" border="0" align="center" cellpadding="0" cellspacing="0">
                      <tr>
                        <td><textarea id="area_user_log" class="log_txt_area" readonly="readonly" ></textarea></td>
                      </tr>
                    </table>
                  </div>
                  <!--tabcon end-->
                  <div class="sublist">
                    <table width="100%" border="0" align="center" cellpadding="0" cellspacing="0">
                      <tr>
                        <td><textarea id="area_sys_log" class="log_txt_area" readonly="readonly" ></textarea></td>
                      </tr>
                    </table>
                  </div>
                  <!--tabcon end-->
                  <div class="sublist">
                    <table width="100%" border="0" align="center" cellpadding="0" cellspacing="0">
                      <tr>
                        <td><textarea id="area_debug_log" class="log_txt_area" readonly="readonly" ></textarea></td>
                      </tr>
                    </table>
                  </div>
                  <!--tabcon end--> 
                </div>
                <!--tabcon end--> 
              </div>
              
              <!--tabbox end--></td>
          </tr>
          <tr>
          	<td align="center" valign="middle" height="30px">
            	<table width="100%" border="0" align="center" cellpadding="0" cellspacing="0">
                  <tr>
                    <td width="1%">&nbsp;</td>
                    <td align="center"><div id="debug_refresh_clear_result">&nbsp;</div></td>
                    <td width="1%">&nbsp;</td>
                  </tr>
                </table>
          	</td>
          <tr>
            <td align="center"><input type="button" value="清空日志" onClick='applog_submit("clear");' />
              &nbsp;&nbsp;
              <input type="button" value="刷新日志" onClick='applog_submit("refresh");'/></td>
          </tr>
        </table>
      </form>
      
      <!-- 内容填充区end --></td>
    <td background="pub/images/mail_rightbg.gif">&nbsp;</td>
  </tr>
  <tr>
    <td valign="bottom" background="pub/images/mail_leftbg.gif"><img src="pub/images/buttom_left2.gif" width="17" height="17" /></td>
    <td background="pub/images/buttom_bgs.gif"><img src="pub/images/buttom_bgs.gif" width="17" height="17"></td>
    <td valign="bottom" background="pub/images/mail_rightbg.gif"><img src="pub/images/buttom_right2.gif" width="16" height="17" /></td>
  </tr>
</table>
</body>
</html>
