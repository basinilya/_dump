// console.log(">> init listing-refresh.js");
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
	function escapeHTML(s) {
	    return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
	}

	function replaceTbody(replacementHtml) {
		var mainTbody = getListingTbody(document);
		if (mainTbody) {
			mainTbody.innerHTML = replacementHtml + "<tr><th colspan=\"5\">" + escapeHTML(new Date() + "") + "</th></tr>";
			var fallbackMutationEvent = new Event("fallbackMutationEvent");
			mainTbody.dispatchEvent(fallbackMutationEvent);
			// console.log("done dispatch mutation event");
		}
	}

	var ifrm;
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
					var targetOrigin = window.location.protocol == "file:" ? "*" : window.location.origin;
					window.parent.postMessage(s, targetOrigin);
					break loop1; 
				}
			}
		}
	}

	function createHelperframe() {
		var thisurlparts = window.location.href.split('#');
		var sep = window.location.search.charAt(0) == "?" ? "&" : "?";
		ifrm = document.createElement('iframe');
		ifrm.id = "helperframe";
		// ifrm.style.cssText = "position:absolute;left:400px;top:10px;width:600px;height:800px;";
		ifrm.style.cssText = "width:0;height:0;border:0; border:none;";
		ifrm.setAttribute("src", thisurlparts[0] + sep + "helperframe=1");
		document.body.appendChild(ifrm);
		console.log("helper frame created");
	}

	if (!isHelperFrame) {
		var refreshSwitch = document.createElement("INPUT");
		refreshSwitch.id = "refreshSwitch";
		refreshSwitch.className = "refreshSwitch";
		refreshSwitch.type = "checkbox";
		refreshSwitch.checked = true;
		refreshSwitch.onchange = function() {
			if (refreshSwitch.checked) {
				createHelperframe();
			} else if (ifrm) {
				document.body.removeChild(ifrm);
				ifrm = null;
			}
		};
		document.body.appendChild(refreshSwitch);
		document.body.appendChild(document.createTextNode("Auto refresh"));
		
		window.addEventListener("message", function(event) {
			if (window.location.protocol == "file:" || window.location.origin == event.origin) {
				if (refreshSwitch.checked) {
					var s = event.data;
					replaceTbody(s);
				}
			}
		});
	}
	
	//if (false)
	window.setTimeout(function() {
		if (isHelperFrame) {
			location.reload();
			return;
		}
	
		if (!refreshSwitch.checked) return;

		createHelperframe();
	}, 2000);
}
// console.log("<< init listing-refresh.js");
