// console.log(">> init listing-player.js 1");
(function() {

	if (window.visualViewport) {
		var windowtop = document.getElementById("windowtop");
		
		function onWindowScroll() {
			windowtop.style.top = window.visualViewport.offsetTop + "px";
			windowtop.style.left = window.visualViewport.offsetLeft + "px";
			windowtop.style.width = window.visualViewport.width + "px";
		}
		onWindowScroll();
		window.visualViewport.addEventListener("resize", onWindowScroll);
		window.visualViewport.addEventListener("scroll", onWindowScroll);
	}

	var setWindowLocationHash = (typeof history.replaceState === 'function')
		? (function(s) {
			if (!s.startsWith("#") && s.length != 0) {
				s = "#" + s;
			}
			/* this will not affect history */
			history.replaceState(undefined, undefined, s);
		})
		: (function(s) {
			window.location.hash = s;
		});

	var listingplayer = document.getElementById("listingplayer");
	var preloadplayer = document.getElementById("preloadplayer");

	var params = window.location.search.substr(1).split("&");
	for (var i in params) {
		var pair = params[i].split("=");
		if (pair[0] == "helperframe" && +pair[1]) {
		    listingplayer.removeAttribute("controls");
		    preloadplayer.removeAttribute("controls");
			return;
		}
	}

	function endsWith(str, suffix) {
	    return str.indexOf(suffix, str.length - suffix.length) !== -1;
	}

	var base = window.location.protocol == "file:" ? "./" : "/";
	var imgpause = base + "static/pause.png";
	var imgplay = base + "static/play.png";

	var currImgEl = null;
	var nextPlayButton = null;

	var onEnded = function() {
		if (nextPlayButton != null) {
			nextPlayButton.onclick();
		}
		listingReload();
	};

	listingplayer.addEventListener("ended", onEnded, false);
	preloadplayer.addEventListener("ended", onEnded, false);

	var onPause = function() {
		var s = listingplayer.src;
		var base = window.location.href.split("#")[0]
		base = base.substring(0, base.lastIndexOf('/')) + '/';
		if (s != null && s.startsWith(base)) {
			s = s.substring(base.length);
		}
		setWindowLocationHash(listingplayer.currentTime + "-" + s);
	};

	listingplayer.addEventListener("pause", onPause, false);
	preloadplayer.addEventListener("pause", onPause, false);

	function insertAfter(newNode, referenceNode) {
	    referenceNode.parentNode.insertBefore(newNode, referenceNode.nextSibling);
	}

	function play(imgEl, _nextPlayButton) {
		onPause();
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
			if (clipUrl == preloadplayer.src) {
				var tmp = preloadplayer;
				preloadplayer = listingplayer;
				listingplayer = tmp;
				listingplayer.setAttribute("controls", "controls");
				preloadplayer.removeAttribute("controls");
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

	var listingTbody = getListingTbody(document);

	var showPlayer = false;

	var pos;
	var clipUrl = "";
	try {
		var arr = window.location.hash.split(/^#([^-]*)-/);
		if (arr.length == 3) {
			pos = arr[1] - 0;
			clipUrl = arr[2];
			var a = document.createElement("a");
			a.setAttribute("href", clipUrl);
			clipUrl = a.href;
		}
	} catch (e) {
		console.error(e);
	}

	function init() {
		// console.log(">> init " + showPlayer);

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
				} else if (clipUrl && clipUrl == href) {
					listingplayer.currentTime = pos;
					listingplayer.src = clipUrl;
					nextPlayButton = _nextPlayButton;
					clipUrl = "";
				}
				_nextPlayButton = createPlayButton(link, href, _nextPlayButton);
				// 
				showPlayer = true;
			}
		}
		
		// console.log("<< init");
	}

	listingTbody.addEventListener("fallbackMutationEvent", init);

	init();
	
	// console.log("showPlayer: " + showPlayer);

	if (showPlayer) {
		listingplayer.setAttribute("controls", "controls");

		var gapdiv = document.createElement("div");
		gapdiv.id = "gapdiv";
		gapdiv.className = "gapdiv";
		document.body.insertBefore(gapdiv, document.body.firstChild);
	} else {
	    listingplayer.removeAttribute("controls");
	}



	document.body.appendChild(document.createTextNode("x (3)"));
})();
// console.log("<< init listing-player.js 1");
