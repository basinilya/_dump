function getListingTbody(iframeDoc) {
	var links = iframeDoc.getElementsByTagName("a");
	for (var i = links.length-1; i >= 0; i--) {
		var link = links[i];
		if (link.innerHTML == "Parent Directory") {
			var tbody = link.parentElement;
			for(;;) {
				if (!tbody) break;
				if (tbody.nodeName == "TBODY") return tbody;
				tbody = tbody.parentElement;
			}
			break;
		}
	}
	return null;
}

{
	
	//if (false)
	window.setTimeout(function() {
		var params = window.location.search.substr(1).split("&");
		for (var i in params) {
			var pair = params[i].split("=");
			if (pair[0] == "helperframe" && +pair[1]) {
				location.reload();
				return;
			}
		}
	
		var thisurlparts = window.location.href.split('#');
		var sep = window.location.search.charAt(0) == "?" ? "&" : "?";
		var ifrm = document.createElement('iframe');
		ifrm.style.cssText = "position:absolute;left:400px;top:10px;width:400px;height:400px;"
		ifrm.setAttribute("src", thisurlparts[0] + sep + "helperframe=1");
		document.body.appendChild(ifrm);
		ifrm.onload = function(){
			console.log("loaded2");
			var iframeDoc = ifrm.contentDocument || ifrm.contentWindow.document;
			var iframeTbody = getListingTbody(iframeDoc);
			if (iframeTbody) {
				var mainTbody = getListingTbody(document);
				if (mainTbody) {
					mainTbody.innerHTML = iframeTbody.innerHTML;
				}
			}
		};
	}, 2000);
}
