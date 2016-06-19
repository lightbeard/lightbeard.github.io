
window.addEventListener('WebComponentsReady', function(e) {
	
	$('body').css('display','block');
	
	// imports are loaded
	var importLink = $('link[href="imports.html"]');
	var menuHtml = $('#dock-menu', importLink[0].import);
	var footer = $('#footer', importLink[0].import);
	$('#main-div').prepend(menuHtml);
	$('#main-div').append(footer);
	
	if(!bowser.mobile && !bowser.tablet) {
		$('#backup-menu').css('display','none');

		$('#dock').css('display','block').Fisheye(
			{
				maxWidth: 100,
				items: 'a',
				itemsText: 'span',
				container: '.dock-container',
				itemWidth: 80,
				proximity: 100,
				halign : 'center'
			}
		);
	} else {
		// TODO: possible screen too small for menu
	}
	
	//$('p').hyphenate('en-us');

	$('.carousel').slick({
		dots: true,
		arrows: false,
		autoplay: true,
		autoplaySpeed: 5000,
		pauseOnDotsHover: true,
		pauseOnFocus: false,
		pauseOnHover: false,
		infinite: true,
		fade: true,
		cssEase: 'linear',
		speed: 500
	});
		
	$('.slick-dots').find('button').on('click', function () {
		$('.carousel').slick('slickPause');
		$('.carousel').slick('setOption', 'autoplay', false, false);
	});
	
	if(window.RESUME) $('#footer').css('display','none');
	else $('#footer-carousel').slick();
	
});
