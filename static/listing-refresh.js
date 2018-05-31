var savedRowsByUrl = [];

function forEachFile(iframeDoc, callback) {
	var links = iframeDoc.getElementsByTagName("a");
	for (var i = links.length-1; i >= 0; i--) {
		var link = links[i];
		if (link.innerHTML == "Parent Directory") {
			var tbody = link.parentElement;
			for(;;) {
				if (!tbody) return;
				if (tbody.nodeName == "TBODY") break;
				tbody = tbody.parentElement;
			}
			console.log("found2");
			var row = tbody.firstChild;
			for(;;) {
				if (!row) break;
				if (row.nodeName == "TR") {
					//console.log(row.innerHTML);
					var td = row.firstChild;
					if (td) {
						td = td.nextSibling;
						if (td) {
							link = td.firstChild;
							if (link) {
								callback(link.href, row);
							}
						}
					}
				}
				row = row.nextSibling;
			}
		}
	}
}

forEachFile(document, function(url, rowElem) {
	savedRowsByUrl[savedRowsByUrl.length] = rowElem.cloneNode(true);
});

window.setTimeout(function() {
	var params = window.location.search.substr(1).split("&");
	for (var i in params) {
		var pair = params[i].split("=");
		if (pair[0] == "helperframe" && +pair[1]) {
			// location.reload();
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
		var iframeRowsByUrl = [];
		forEachFile(iframeDoc, function(url, rowElem) {
			iframeRowsByUrl[iframeRowsByUrl.length] = rowElem;
		});
		var iSrc = 0;
		var iDst = 0;
		for (var url in savedRowsByUrl) {
			//if (!)
		}
	};
}, 1000);