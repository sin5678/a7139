/* rf433 js script */
String.prototype.replaceAll = function(oldStr, newStr) {
    return this.replace(new RegExp(oldStr,"gm"),newStr); 
}

function ret_check(ret)
{
    if (ret.indexOf("<html>") == -1) {
        return true;
    } else {
        return false;
    }
}

function lang_get_cn(str)
{
    var result = str;
    result = result.replaceAll('addr', '传感器地址');
    result = result.replaceAll('req_cnt', '请求次数');
    result = result.replaceAll('rsp_cnt', '响应次数');
    result = result.replaceAll('type', '类型');
    result = result.replaceAll('data', '数据');
    result = result.replaceAll('vol', '电压');
    result = result.replaceAll('batt', '电池');
    result = result.replaceAll('flag', '状态');
    result = result.replaceAll('whid', '巡更');
    return result;
}

function update_table(data,itemid,s1,s2)
{
    var items_v1=data.split(s1);
    var tbl_head="";
    var tbl_body="";
    for (var i=0;i<items_v1.length;i++) {
        var tbl_line="";
        var items_v2=items_v1[i].split(s2);
        for (var j=0;j<items_v2.length;j++) {
            var pos=items_v2[j].indexOf("=");
            if (pos==-1) continue;
            var item=items_v2[j].substring(0,pos);
            var vale=items_v2[j].substring(pos+1);
            if (i==0) {
                tbl_head=tbl_head+"<th>"+lang_get_cn(item)+"</th>";
            }
            tbl_line=tbl_line+"<td align='center'>"+vale+"</td>";
        }
        if (i%2==0) {
            tbl_body=tbl_body+"<tr class=\"t1\">"+tbl_line+"</tr>";
        } else {
            tbl_body=tbl_body+"<tr>"+tbl_line+"</tr>";
        }
    }
    if (i<=1) tbl_body="<tr><td align=\"center\">暂无传感器</td></tr>";
    $("table#"+itemid).append("<thead>"+tbl_head+"</thead>"+"<tbody>"+tbl_body+"</tbody>");
}

$(function(){
    $("#se433_btn").click(function(){
        $("table#se433_list_tbl").empty();
        $.ajax({
            type:"GET",
            cache:false,
            url:"/cgi-bin/rf433.cgi",
            timeout:3000,
            async:false,
            success:function(ret){
                if (ret_check(ret)) {
                    update_table(ret, "se433_list_tbl", "\n", ", ");
                } else {
                    $("table#se433_list_tbl").html("<tr><td align=\"center\">刷新433活动传感器列表失败!</td></tr>");
                }
            },
            error:function(ret){
                $("table#se433_list_tbl").html("<tr><td align=\"center\">刷新433活动传感器列表失败!</td></tr>");
                //alert("刷新433活动传感器列表失败!");
            },
        });
    });
    $("#se433_btn").click();
});

