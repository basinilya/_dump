{
	function endsWith(str, suffix) {
	    return str.indexOf(suffix, str.length - suffix.length) !== -1;
	}

	function startsWith(str, prefix) {
	    return str.lastIndexOf(prefix, 0) === 0;
	}
	
	var base = "./";
	var imgpause = base + "pause.png";
	var imgplay = base + "play.png";

	var listingplayer = document.getElementById("listingplayer");
	var preloadplayer = document.getElementById("preloadplayer");

	var currImgEl = null;
	var nextPlayButton = null;

	listingplayer.addEventListener("ended", function () {
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
		_nextPlayButton.preloadClip();
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
		imgEl.src = imgplay;
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

	{
		var nextPlayButton = null;
		var links = document.getElementsByTagName("a");
		var showPlayer = false;
		for (var i = links.length-1; i >= 0; i--) {
			var link = links[i];
			var href = link.href; 
			if (typeof href !== 'undefined' && endsWith(href, ".mp3")) {
				nextPlayButton = createPlayButton(link, href, nextPlayButton);
				showPlayer = true;
			}
		}

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
}
