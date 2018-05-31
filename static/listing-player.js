{
	function endsWith(str, suffix) {
	    return str.indexOf(suffix, str.length - suffix.length) !== -1;
	}

	function startsWith(str, prefix) {
	    return str.lastIndexOf(prefix, 0) === 0;
	}
	
	var base = startsWith(window.location.href, "file:") ? "./" : "/";
	var imgpause = base + "static/pause.png";
	var imgplay = base + "static/play.png";

	var listingplayer = document.getElementById("listingplayer");
	var preloadplayer = document.getElementById("preloadplayer");

	var currImgEl = null;
	var nextPlayButton = null;

	listingplayer.addEventListener("ended", function() {
		if (nextPlayButton != null) {
			nextPlayButton.onclick();
		}
	}, false);                                                                               

	function insertAfter(newNode, referenceNode) {
	    referenceNode.parentNode.insertBefore(newNode, referenceNode.nextSibling);
	}

	function play(imgEl, _nextPlayButton) {
		listingplayer.play();
		imgEl.src = imgpause;
		currImgEl = imgEl;
		if (_nextPlayButton != null) {
			_nextPlayButton.preloadClip();
		}
		nextPlayButton = _nextPlayButton;
	}

	function createPlayButton(link, clipUrl, _nextPlayButton) {
		var playButton;
	
		playButton = document.createElement("a");
		playButton.setAttribute("class", "playerButton");
		playButton.href = "javascript:void(0)";
	
		insertAfter(playButton, link);

		var imgEl;
		imgEl = document.createElement("img");
		var isPlaying = listingplayer.src == clipUrl && !listingplayer.paused;
		if (isPlaying) {
			currImgEl = imgEl;
		}
		imgEl.src = isPlaying ? imgpause : imgplay;
		playButton.appendChild(imgEl);
	
		playButton.preloadClip = function() {
			preloadplayer.src = clipUrl;
			preloadplayer.load();
		}

		playButton.onclick = function() {
			if (currImgEl != null) {
				currImgEl.src = imgplay;
				currImgEl = null;
			}
			if (clipUrl == listingplayer.src) {
				// invert
				if (listingplayer.paused) {
					play(imgEl, _nextPlayButton);
				} else {
					listingplayer.pause();
				}
			} else {
				listingplayer.src = clipUrl;
				play(imgEl, _nextPlayButton);
			}
			console.log(clipUrl);
		};

		return playButton;
	}

	var myConfig = { childList: true };

	var myObserver = new MutationObserver(function(mutations, myObserver) {
        mutations.forEach(function(mutation) {
        	if (mutation.type == "childList") {
        		// console.log(">> mutation");
        		init();
        		// console.log("<< mutation");
        	}
        });
	});

	var myElem = getListingTbody(document);

	var showPlayer = false;

	function init() {
		var params = window.location.search.substr(1).split("&");
		for (var i in params) {
			var pair = params[i].split("=");
			if (pair[0] == "helperframe" && +pair[1]) {
			    listingplayer.removeAttribute("controls");
				return;
			}
		}
		
		// console.log(">> init");
		
		myObserver.disconnect();

		currImgEl = null;
		nextPlayButton = null;

		var _nextPlayButton = null;
		var links = document.getElementsByTagName("a");
		for (var i = links.length-1; i >= 0; i--) {
			var link = links[i];
			var href = link.href; 
			if (typeof href !== 'undefined' && endsWith(href, ".mp3")) {
				if (listingplayer.src == href) {
					nextPlayButton = _nextPlayButton;
				}
				_nextPlayButton = createPlayButton(link, href, _nextPlayButton);
				// 
				showPlayer = true;
			}
		}
		
		if (myElem) {
			myObserver.observe(myElem, myConfig);
		}

		// console.log("<< init");
	}

	init();

	if (showPlayer) {
		listingplayer.setAttribute("controls", "controls");

		var gapdiv = document.createElement("div");
		gapdiv.id = "gapdiv";
		gapdiv.className = "gapdiv";
		document.body.insertBefore(gapdiv, document.body.firstChild);
	} else {
	    listingplayer.removeAttribute("controls");
	}
}
