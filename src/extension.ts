// The module 'vscode' contains the VS Code extensibility API
// Import the module and reference it with the alias vscode in your code below
import * as vscode from 'vscode';

import { spawn } from 'child_process';

interface IMessage {
	readonly jsonrpc: string;

	toString(): string;
}

class RPCMessage implements IMessage {
	jsonrpc: string = '2.0';
	method: string;
	params: [any];

	constructor(method: string, params: [any]) {
		this.method = method;
		this.params = params;
	}

	toString() {
		return JSON.stringify({
			jsonrpc: this.jsonrpc,
			method: this.method,
			params: this.params,
		});
	}
}

class KeysMessage extends RPCMessage {
	constructor(keys: string) {
		keys = keys.replace('\n', '<ret>');
		super('keys', [keys]);
	}
}

// this method is called when your extension is activated
// your extension is activated the very first time the command is executed
export function activate(context: vscode.ExtensionContext) {

	// Use the console to output diagnostic information (console.log) and errors (console.error)
	// This line of code will only be executed once when your extension is activated
	console.log('Congratulations, your extension "kakoune-mode" is now active!');

	// The command has been defined in the package.json file
	// Now provide the implementation of the command with registerCommand
	// The commandId parameter must match the command field in package.json
	let disposable = vscode.commands.registerCommand('extension.helloWorld', () => {
		// The code you place here will be executed every time your command is executed

		// Display a message box to the user
		vscode.window.showInformationMessage('Hello World!');
	});

	context.subscriptions.push(disposable);

	// Clear any previously used instances.
	spawn('kak', ['-clear']);

	// Start kakoune
	const kak = spawn('kak', ['-ui', 'json', '-s', 'vscode']);
	kak.stdout.on('data', (data) => {
		console.log(`${data}`);
	});

	kak.stderr.on('data', (data) => {
		vscode.window.showErrorMessage(`Failed to start Kakoune daemon: ${data}`);
	});

	overrideCommand(context, 'type', args => {
		if (!args.text) { return; }

		console.log(args.text);
		const msg = new KeysMessage(args.text);
		console.log(msg.toString());

		kak.stdin.write(msg.toString());
	});

	const sendEscape = vscode.commands.registerCommand('extension.send_escape', () => {
		const msg = new KeysMessage('<esc>');

		console.log(msg.toString());
		kak.stdin.write(msg.toString());
	});
	context.subscriptions.push(sendEscape);

	const sendBackspace = vscode.commands.registerCommand('extension.send_backspace', () => {
		const msg = new KeysMessage('<backspace>');

		console.log(msg.toString());
		kak.stdin.write(msg.toString());
	});
	context.subscriptions.push(sendEscape);
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
