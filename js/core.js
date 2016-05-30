
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

$('p').hyphenate('en-us');