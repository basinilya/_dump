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
	console.log("Not found table containing file list");
	return null;
}

{
	var isHelperFrame = false;
	{
		var params = window.location.search.substr(1).split("&");
		loop1:
		for (var i in params) {
			var pair = params[i].split("=");
			if (pair[0] == "helperframe" && +pair[1]) {
				isHelperFrame = true;
				var mainTbody = getListingTbody(document);
				if (mainTbody) {
					var s = mainTbody.innerHTML;
					targetOrigin = window.location.protocol == "file:" ? "*" : window.location.origin;
					window.parent.postMessage(s, targetOrigin);
					break loop1; 
				}
			}
		}
	}

	function replaceTbody(replacementHtml) {
		var mainTbody = getListingTbody(document);
		if (mainTbody) {
			mainTbody.innerHTML = replacementHtml + "<tr><th colspan=\"5\">" + new Date() + "</th></tr>";
		}
	}

	if (!isHelperFrame) {
		window.addEventListener("message", function(event) {
			if (window.location.protocol == "file:" || window.location.origin == event.origin) {
				var s = event.data;
				replaceTbody(s);
			}
		});
	}
	
	//if (false)
	window.setTimeout(function() {
		if (isHelperFrame) {
			location.reload();
			return;
		}
	
		var thisurlparts = window.location.href.split('#');
		var sep = window.location.search.charAt(0) == "?" ? "&" : "?";
		var ifrm = document.createElement('iframe');
		// ifrm.style.cssText = "position:absolute;left:400px;top:10px;width:600px;height:800px;";
		ifrm.style.cssText = "width:0;height:0;border:0; border:none;";
		ifrm.setAttribute("src", thisurlparts[0] + sep + "helperframe=1");
		document.body.appendChild(ifrm);
		ifrm.onload = function(){
			// console.log("loaded2");
			if (false) {
				var iframeDoc = ifrm.contentDocument || ifrm.contentWindow.document;
				var iframeTbody = getListingTbody(iframeDoc);
				if (iframeTbody) {
					replaceTbody(iframeTbody.innerHTML);
				}
			}
		};
	}, 2000);
}
