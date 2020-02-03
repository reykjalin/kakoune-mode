// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';

import { ChildProcess, spawn } from 'child_process';

import {
	getMode,
	handleCommand,
} from './external/Extension.js';
import { createKeysMessage } from './external/Rpc.js';
import { showError } from './external/VSCode.js';

const startKakoune = (): ChildProcess => {
	// Clear any previously used instances.
	spawn( 'kak', ['-clear'] );

	const currentFile = vscode.window.activeTextEditor?.document.fileName || '';
	const kak = spawn( 'kak', ['-ui', 'json', '-s', 'vscode', currentFile] );
	kak.stdout.on( 'data', handleCommand );
	kak.stderr.on( 'data', showError );
	return kak;
};

// this method is called when your extension is activated
// your extension is activated the very first time the command is executed
export const activate = ( context: vscode.ExtensionContext ) => {

	// Use the console to output diagnostic information (console.log) and errors (console.error)
	// This line of code will only be executed once when your extension is activated
	if ( vscode.window.activeTextEditor ) {
		vscode.window.activeTextEditor.options = { cursorStyle: vscode.TextEditorCursorStyle.Block };
	}

	// Start kakoune
	const kak = startKakoune();

	// Override the typing command when in a text document.
	overrideCommand( context, 'type', args => {
		if ( !args.text ) { return; }

		const msg = createKeysMessage( args.text );
		kak.stdin.write( JSON.stringify( msg ) );
	} );

	// Override the escape command.
	const sendEscape = vscode.commands.registerCommand( 'extension.send_escape', () => {
		const msg = createKeysMessage( '<esc>' );
		kak.stdin.write( JSON.stringify( msg ) );
	} );
	context.subscriptions.push( sendEscape );

	// Override the backspace command.
	const sendBackspace = vscode.commands.registerCommand( 'extension.send_backspace', () => {
		const msg = createKeysMessage( '<backspace>' );
		kak.stdin.write( JSON.stringify( msg ) );

		// Make sure to erase character when in insert mode.
		if ( 'Insert' === getMode().name ) {
			vscode.commands.executeCommand( 'deleteLeft' );
		}
	} );
	context.subscriptions.push( sendBackspace );

	// Make sure a documents are opened in kakoune.
	vscode.window.onDidChangeActiveTextEditor( event => {
		if ( !event ) {
			return;
		}

		console.log( event.document.fileName );

		const msg = createKeysMessage( `:e ${event.document.fileName}<ret>` );
		kak.stdin.write( JSON.stringify( msg ) );
	} );
};

const overrideCommand = (
	context: vscode.ExtensionContext,
	command: string,
	callback: ( ...args: any[] ) => any,
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

// this method is called when your extension is deactivated
export const deactivate = () => { };
