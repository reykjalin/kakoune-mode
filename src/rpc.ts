import * as vscode from 'vscode';
import {
    WorkspaceEdit,
    TextEdit,
    Selection,
    Position,
} from 'vscode';

export class RPCMessage {
    jsonrpc: string = '2.0';
    method: string;
    params: [any];

    constructor(method: string, params: [any]) {
        this.method = method;
        this.params = params;
    }
}

export class KeysMessage extends RPCMessage {
    constructor(keys: string) {
        keys = keys.replace('\n', '<ret>');
        super('keys', [keys]);
    }
}

type rpcmsg = {
    jsonrpc: string,
    method: string,
    params: any,
};

type drawCommand = [
    line[],
    face,
    face,
];

type line = atom[];

type atom = {
    contents: string,
    face: face,
};

type face = {
    fg: string,
    bg: string,
};

/**
 * Handle incoming commands from Kakoune.
 * 
 * @param data The message sent from Kakoune via stdout.
 */
export const handleIncomingCommand = (data: { type: string, data: [] }): void => {
    const dataStr = `${data}`;
    const command = getDrawCommand(dataStr);

    // If no editor is selected, just return; no need to handle the data.
    const { activeTextEditor } = vscode.window;
    if (!activeTextEditor) {
        return;
    }

    const newLines = command.reduce(commandToLines, []);

    // Prepare edits to editor.
    const edits = newLines.map(lineToEdit);
    const workEdits = new WorkspaceEdit();
    workEdits.set(activeTextEditor.document.uri, onlyEdits(edits));

    // Prepare selections in editor.
    const selections = linesToSelections(newLines);

    // Apply the edits and make the selections.
    vscode.workspace.applyEdit(workEdits).then(() => {
        if (selections.length > 0) {
            activeTextEditor.selections = selections;
        }
    });
};

/**
 * Return the most recent draw command.
 * 
 * @param msg JSON rpc message.
 * 
 * @returns The newest draw command.
 */
const getDrawCommand = (msg: string): drawCommand[] => {
    return msg.split('\n').
        // No empty messages.
        filter((msg: string): boolean => '' !== msg).
        // Parse to a JS object.
        map((msg: string): rpcmsg => JSON.parse(msg)).
        // Only get draw messages.
        filter((msg: rpcmsg): boolean => 'draw' === msg.method).
        // Retrieve the parameters.
        map((msg: rpcmsg): drawCommand => msg.params).
        // Only the last draw command.
        slice(-1);
};

/**
 * Reduces draw information for a whole document to an array of lines.
 * 
 * @param accumulatedLines The list of lines.
 * @param currentCommand   The command to reduce to a line.
 * 
 * @returns A list of lines to draw.
 */
const commandToLines = (accumulatedLines: line[], currentCommand: drawCommand): line[] => {
    return [
        ...accumulatedLines,
        ...currentCommand[0]
    ];
};

/**
 * Map function used to map lines to a VSCode TextEdit.
 * 
 * @param line  Line to turn into an edit.
 * @param index Index of the current line.
 * 
 * @returns A VSCode TextEdit.
 */
const lineToEdit = (line: line, index: number): TextEdit | undefined => {
    const { activeTextEditor } = vscode.window;
    if (!activeTextEditor) {
        return undefined;
    }
    const documentLine = activeTextEditor.document.lineAt(index);
    const documentLineRange = documentLine.rangeIncludingLineBreak;
    const newContent = line.reduce(mergeLineContents, '');

    if (newContent === documentLine.text) {
        return undefined;
    }

    return TextEdit.replace(documentLineRange, newContent);
};

/**
 * Reducer that merges atoms that form a line into a string that represents the line.
 * 
 * @param lineContent The line content.
 * @param currentAtom The atom to be added to the line content.
 * 
 * @returns The line content from all the Atoms in the line.
 */
const mergeLineContents = (lineContent: string, currentAtom: atom): string => lineContent + currentAtom.contents;

/**
 * Hacky way to get only defined edits. For some reason TS doesn't recognize
 * this when using .filter() on an array.
 * 
 * @param edits List of edits.
 * 
 * @returns A list of valid VSCode TextEdits.
 */
const onlyEdits = (edits: (TextEdit | undefined)[]): TextEdit[] => {
    const validEdits: TextEdit[] = [];
    edits.forEach(edit => {
        if (undefined !== edit) {
            validEdits.push(edit);
        }
    });
    return validEdits;
};

/**
 * Convert a list of lines to a list of VSCode Selections.
 * 
 * @param lines The list of lines.
 * 
 * @returns A list of VSCode Selections corresponding to the input lines.
 */
const linesToSelections = (lines: line[]): Selection[] => {
    let cursorPos: Position | undefined = undefined;
    let selectionStart: Position | undefined = undefined;
    let pos: number = 0;

    lines.forEach((line, index) => {
        line.forEach(atom => {
            if ('black' === atom.face.fg) {
                // Add the cursor position if it hasn't been defined already.
                if (!cursorPos) {
                    cursorPos = new Position(index, pos);
                }
            } else if ('white' === atom.face.fg) {
                // selection
                if (cursorPos) {
                    selectionStart = new Position(index, pos + atom.contents.length);
                } else {
                    // Only move the selection start if it hasn't been defined already.
                    // This is beause the selection may have multiple lines in it.
                    if (!selectionStart) {
                        selectionStart = new Position(index, pos);
                    }
                }
            }
            pos += atom.contents.length;
        });
        pos = 0;
    });

    if (selectionStart && cursorPos) {
        const selection = new Selection(selectionStart, cursorPos);
        return [
            selection
        ];
    } else if (selectionStart) {
        const selection = new Selection(selectionStart, selectionStart);
        return [
            selection
        ];
    } else if (cursorPos) {
        const selection = new Selection(cursorPos, cursorPos);
        return [
            selection
        ];
    }
    return [];
};