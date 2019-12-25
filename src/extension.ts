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
	if (vscode.window.activeTextEditor) {
		vscode.window.activeTextEditor.options = { cursorStyle: vscode.TextEditorCursorStyle.Block };
	}

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
	const currentFile = vscode.window.activeTextEditor?.document.fileName || '';
	const kak = spawn('kak', ['-ui', 'json', '-s', 'vscode', currentFile]);
	kak.stdout.on('data', (data) => {
		// console.log(`${data}`);

		`${data}`.split('\n').filter(message => '' !== message).forEach(message => {
			const msg = JSON.parse(message);
			if ('draw' === msg.method) {
				if (!vscode.window.activeTextEditor) {
					return;
				}
				const activeEditor = vscode.window.activeTextEditor;

				const edits: vscode.TextEdit[] = [];

				// Lines are first by definition.
				const lines: [[{ contents: string }]] = msg.params[0];
				lines.forEach((line, index) => {

					const newContent: string = line.reduce(
						(accumulator, currentValue) => {
							return accumulator + currentValue.contents;
						},
						''
					);

					const currentLine = activeEditor.document.lineAt(index);
					const currentLineRange = currentLine.rangeIncludingLineBreak;
					edits.push(vscode.TextEdit.replace(currentLineRange, newContent));
				});
				const workEdits = new vscode.WorkspaceEdit();
				workEdits.set(activeEditor.document.uri, edits);
				vscode.workspace.applyEdit(workEdits);
			} else if ('set_cursor' === msg.method) {
				if (!vscode.window.activeTextEditor) {
					return;
				}
				console.log(msg);
				const activeEditor = vscode.window.activeTextEditor;
				const column = msg.params[1].column;
				const line = msg.params[1].line;
				console.log('line:', line);
				console.log('column:', column);
				const cursorPos = new vscode.Position(line, column);
				activeEditor.selections = [new vscode.Selection(cursorPos, cursorPos)];
			}
		});
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
	context.subscriptions.push(sendBackspace);

	vscode.window.onDidChangeActiveTextEditor(event => {
		if (!event) {
			return;
		}

		console.log(event.document.fileName);

		const msg = new KeysMessage(`:e ${event.document.fileName}<ret>`);
		kak.stdin.write(msg.toString());
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
