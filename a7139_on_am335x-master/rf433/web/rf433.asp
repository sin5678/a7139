<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<link href="pub/css/skin.css" rel="stylesheet" type="text/css" />
<link href="pub/css/validator.css" type="text/css" rel="stylesheet"/>
<title>433无线设置</title>
<script src="pub/js/jquery.min.js" type="text/javascript"></script>
<script src="pub/formvalidator/formvalidator.min.js" type="text/javascript"></script>
<script src="pub/formvalidator/formvalidatorregex.js" type="text/javascript"></script>
<script src="pub/js/common.js" type="text/javascript"></script>
<script type="text/javascript">
$(document).ready(function(){
        $.formValidator.initConfig({formID:"rf433form",theme:'default',onError:function(msg){alert(msg); $("#resultdiv").hide();}}); 
        $("#rf433_net_id").formValidator({tipID:"rf433_net_id_tip",onShow:"",onFocus:"无线网络ID必须在0-255之间",onCorrect:"",onError:"无线网络ID必须在0-255之间"}).inputValidator({min:0,max:255,type:"value",onError:"无线网络ID必须在0-255之间"}).defaultPassed();
		// bug#63
		$("#rf433_local_addr").formValidator({tipID:"rf433_local_addr_tip",onShow:"",onFocus:"以0x00或0X00开头的十六进制数，且长度必须为10。例如:0x00abcd12;0X0000A1b2",onCorrect:""}).inputValidator({min:1,max:10,empty:{leftEmpty:false,rightEmpty:false,emptyError:"密码两边不能有空符号"},onError:"无线地址不能为空,请确认"}).regexValidator({regExp:"hex1",dataType:"enum",onError:"以0x00或0X00开头的十六进制数，且长度必须为10。例如:0x00abcd12;0X0000A1b2"}).defaultPassed();
        $("#rf433_server_ip").formValidator({tipID:"rf433_server_ip_tip",onShow:"",onFocus:"格式例如:192.168.1.1",onCorrect:""}).regexValidator({regExp:"ip4",dataType:"enum",onError:"服务器IP格式不正确"});
        $("#rf433_server_port").formValidator({tipID:"rf433_server_port_tip",onShow:"",onFocus:"端口必须在1000-65535之间",onCorrect:"",onError:"端口必须在1000-65535之间"}).inputValidator({min:1000,max:65535,type:"value",onError:"端口必须在1000-65535之间"}).defaultPassed();
        });
</script>
</head>

<body onLoad="checkresult('resultdiv');">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
<tr>
    <td width="17" valign="top" background="pub/images/mail_leftbg.gif"><img src="pub/images/left-top-right.gif" width="17" height="29" /></td>
    <td valign="top" background="pub/images/content-bg.gif">
        <table width="100%" border="0" cellpadding="0" cellspacing="0" class="left_topbg" id="table2">
            <tr> <td height="31"><div class="titlebt2">433无线设置</div></td> </tr>
        </table>
    </td>
    <td width="16" valign="top" background="pub/images/mail_rightbg.gif"><img src="pub/images/nav-right-bg.gif" width="16" height="29" /></td>
</tr>
<tr>
    <td valign="middle" background="pub/images/mail_leftbg.gif">&nbsp;</td>
    <td valign="top" height="100%" width="100%" bgcolor="#F7F8F9">
    <!-- 内容填充区begin -->






<form action="/action/setrf433config" method="post" id="rf433form" name="rf433form">
    <table width="95%" border="0" align="center" cellpadding="0" cellspacing="0">
        <tr> <td class="left_txt">当前位置：433无线设置</td> </tr>
        <tr>
            <td height="20">
                <table width="100%" height="1" border="0" cellpadding="0" cellspacing="0" bgcolor="#CCCCCC">
                    <tr>
                        <td></td>
                    </tr>
                </table>
            </td>
        </tr>
        <tr>
            <td>
                <table width="100%" height="55" border="0" cellpadding="0" cellspacing="0">
                    <tr>
                        <td width="10%" height="55"><img src="pub/images/mime.gif" width="54" height="55"></td>
                        <td width="90%"><span class="left_txt2">在这里，您可以查看和修改433无线网络转发功能的</span><span class="left_txt3">全部配置参数</span><span class="left_txt2">！</span><br>
                            <span class="left_txt2">包括</span><span class="left_txt3">433无线网络ID，433无线本机地址，服务器IP，服务器端口</span><span class="left_txt2">等</span><br>
                            <span class="left_txt2">
                                <font color="red">注意：<br>
                                    1、保存配置后，重启方能生效。</font></span>
                        </td>
                    </tr>
                </table>
            </td>
        </tr>
        <tr>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>
                <table width="100%" border="0" cellpadding="0" cellspacing="0" class="nowtable tbl">
                    <tr>
                        <td class="left_bt2">433无线配置</td>
                    </tr>
                </table>
            </td>
        </tr>
        <tr>
            <td>
                <table width="100%" border="0" cellspacing="0" cellpadding="0" class="left_txt2 tbl">
                    <tr class="t1">
                        <td align="right" class="left_txt">433无线网络ID：</td>
                        <td>&nbsp;</td>
                        <td> <input name="rf433_net_id" id="rf433_net_id" type="text" value="<% setting_get("rf433_net_id"); %>"> </td>
                        <td> <div id="rf433_net_id_tip" style="width:280px"></div> </td>
                    </tr>
                    <tr>
                        <td align="right" class="left_txt">433无线本机地址：</td>
                        <td>&nbsp;</td>
                        <td> <input name="rf433_local_addr" id="rf433_local_addr" type="text" maxlength="10" value="<% setting_get("rf433_local_addr"); %>"> </td>
                        <td> <div id="rf433_local_addr_tip" style="width:280px"></div> </td>
                    </tr>
                    <tr class="t1">
                        <td align="right" class="left_txt">服务器IP：</td>
                        <td>&nbsp;</td>
                        <td> <input name="rf433_server_ip" id="rf433_server_ip" type="text" value="<% setting_get("rf433_server_ip"); %>"> </td>
                        <td> <div id="rf433_server_ip_tip" style="width:280px"></div> </td>
                    </tr>
                    <tr>
                        <td align="right" class="left_txt">服务器端口：</td>
                        <td>&nbsp;</td>
                        <td> <input name="rf433_server_port" id="rf433_server_port" type="text" value="<% setting_get("rf433_server_port"); %>"> </td>
                        <td> <div id="rf433_server_port_tip" style="width:280px"></div> </td>
                    </tr>
                    <tr>
                        <td align="right" class="left_txt">&nbsp;</td>
                        <td>&nbsp;</td>
                        <td colspan="2">
                        	<div id="resultdiv" class="result_fail_txt"></div>
                        </td>
                    </tr>
                    <tr>
                        <td>&nbsp;</td>
                        <td>&nbsp;</td>
                        <td> <input type="submit" value="保存" id="button" name="button"> </td>
                        <td>&nbsp;</td>
                    </tr>
                </table>
            </td>
        </tr>
        <tr>
            <td>&nbsp;</td>
        </tr>
        <tr>
            <td>
                <table width="100%" border="0" cellpadding="0" cellspacing="0" class="nowtable tbl">
                    <tr>
                        <td class="left_bt2">&nbsp;&nbsp;&nbsp;&nbsp;433活动传感器列表</td>
                    </tr>
                </table>
            </td>
        </tr>
        <tr>
            <td>
                <table width="100%" border="0" cellspacing="0" cellpadding="0" class="left_txt2 tbl" id="se433_list_tbl"> </table>
            </td>
        </tr>
        <tr><td align="right"><input type="button" value="刷新" id="se433_btn" name="se433_btn"/>&nbsp;&nbsp;</td></tr>
    </table>
</form>

    <!-- 内容填充区end -->
    </td>
    <td background="pub/images/mail_rightbg.gif">&nbsp;</td>
</tr>
<tr>
    <td valign="bottom" background="pub/images/mail_leftbg.gif"><img src="pub/images/buttom_left2.gif" width="17" height="17" /></td>
    <td background="pub/images/buttom_bgs.gif"><img src="pub/images/buttom_bgs.gif" width="17" height="17"></td>
    <td valign="bottom" background="pub/images/mail_rightbg.gif"><img src="pub/images/buttom_right2.gif" width="16" height="17" /></td>
</tr>
</table>
<script src="pub/js/user/rf433.js" type="text/javascript"></script>
</body>
</html>
