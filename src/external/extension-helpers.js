const vscode = require( 'vscode' );
const { createKeysMessage } = require( './Rpc.js' );

const overrideCommand = (
	context,
	getMode,
	command,
	callback,
) => {
	const disposable = vscode.commands.registerCommand( command, async args => {
		if ( !vscode.window.activeTextEditor ) {
			return;
		}

		if (
			vscode.window.activeTextEditor.document &&
			vscode.window.activeTextEditor.document.uri.toString() === 'debug:input'
		) {
			return vscode.commands.executeCommand( 'default:' + command, args );
		}

		if ( 'Insert' === getMode().name ) {
			vscode.commands.executeCommand( 'default:' + command, args );
		}
		return callback( args );
	} );
	context.subscriptions.push( disposable );
};

const overrideTypeCommand = ( context, getMode, writeToKak ) => {
	overrideCommand(
		context,
		getMode,
		'type',
		args => {
			if ( !args.text ) { return; }

			const msg = createKeysMessage( args.text );
			writeToKak( JSON.stringify( msg ) )
		}
	);
};

const setCursorStyleToBlock = ( activeTextEditor ) => {
	if ( activeTextEditor ) {
		activeTextEditor.options = { cursorStyle: vscode.TextEditorCursorStyle.Block };
	}
};

const registerWindowChangeEventHandler = ( writeToKak ) => {
	// Make sure a documents are opened in kakoune.
	vscode.window.onDidChangeActiveTextEditor( event => {
		if ( !event ) {
			return;
		}

		console.log( event.document.fileName );
		const msg = createKeysMessage( `:e ${event.document.fileName}<ret>` );
		writeToKak( JSON.stringify( msg ) );
	} )
}

module.exports = {
	overrideTypeCommand,
	setCursorStyleToBlock,
	registerWindowChangeEventHandler,
};