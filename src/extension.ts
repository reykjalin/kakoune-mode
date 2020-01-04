// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';
import { Position, Selection } from 'vscode';

import { spawn } from 'child_process';
import { KeysMessage, handleIncomingCommand } from './rpc';

class Cursor {
	mode: string;
	line: number;
	column: number;

	constructor(cursorParam: [string, { line: number, column: number }]) {
		this.mode = cursorParam[0];
		this.line = cursorParam[1].line;
		this.column = cursorParam[1].column;
	}

	getPos(): Position {
		return new Position(this.line, this.column);
	}

	getSelection(): Selection {
		return new Selection(this.getPos(), this.getPos());
	}
}

// this method is called when your extension is activated
// your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

	// Use the console to output diagnostic information (console.log) and errors (console.error)
	// This line of code will only be executed once when your extension is activated
	if (vscode.window.activeTextEditor) {
		vscode.window.activeTextEditor.options = { cursorStyle: vscode.TextEditorCursorStyle.Block };
	}

	// Clear any previously used instances.
	spawn('kak', ['-clear']);

	// Start kakoune
	const currentFile = vscode.window.activeTextEditor?.document.fileName || '';
	const kak = spawn('kak', ['-ui', 'json', '-s', 'vscode', currentFile]);
	kak.stdout.on('data', handleIncomingCommand);

	kak.stderr.on('data', (data) => {
		vscode.window.showErrorMessage(`Kakoune encountered an error: ${data}`);
	});

	// Override the typing command when in a text document.
	overrideCommand(context, 'type', args => {
		if (!args.text) { return; }

		const msg = new KeysMessage(args.text);
		kak.stdin.write(JSON.stringify(msg));
	});

	// Override the escape command.
	const sendEscape = vscode.commands.registerCommand('extension.send_escape', () => {
		const msg = new KeysMessage('<esc>');

		kak.stdin.write(JSON.stringify(msg));
	});
	context.subscriptions.push(sendEscape);

	// Override the backspace command.
	const sendBackspace = vscode.commands.registerCommand('extension.send_backspace', () => {
		const msg = new KeysMessage('<backspace>');

		kak.stdin.write(JSON.stringify(msg));
	});
	context.subscriptions.push(sendBackspace);

	// Make sure a documents are opened in kakoune.
	vscode.window.onDidChangeActiveTextEditor(event => {
		if (!event) {
			return;
		}

		console.log(event.document.fileName);

		const msg = new KeysMessage(`:e ${event.document.fileName}<ret>`);
		kak.stdin.write(JSON.stringify(msg));
	});
}

function overrideCommand(
	context: vscode.ExtensionContext,
	command: string,
	callback: (...args: any[]) => any
) {
	const disposable = vscode.commands.registerCommand(command, async args => {
		if (!vscode.window.activeTextEditor) {
			return;
		}

		if (
			vscode.window.activeTextEditor.document &&
			vscode.window.activeTextEditor.document.uri.toString() === 'debug:input'
		) {
			return vscode.commands.executeCommand('default:' + command, args);
		}

		return callback(args);
	});
	context.subscriptions.push(disposable);
}

// this method is called when your extension is deactivated
export function deactivate() { }
