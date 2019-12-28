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

export const handleIncomingCommand = (data: { type: string, data: [] }): void => {
    const dataStr = `${data}`;
    const command = getDrawCommand(dataStr);

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
 */
const getDrawCommand = (msg: string): drawCommand[] => {
    return msg.split('\n').
        filter((msg: string): boolean => '' !== msg).
        map((msg: string): rpcmsg => JSON.parse(msg)).
        filter((msg: rpcmsg): boolean => 'draw' === msg.method).
        map((msg: rpcmsg): drawCommand => msg.params).
        slice(-1);
};

const commandToLines = (accumulatedLines: line[], currentCommand: drawCommand): line[] => {
    return [
        ...accumulatedLines,
        ...currentCommand[0]
    ];
};

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

const mergeLineContents = (lineContent: string, currentAtom: atom): string => lineContent + currentAtom.contents;

/**
 * Hacky way to get only defined edits. For some reason TS doesn't recognize
 * this when using .filter() on an array.
 * 
 * @param edits List of edits.
 */
const onlyEdits = (edits: (TextEdit | undefined)[]): TextEdit[] => {
    const validEdits: TextEdit[] = [];
    edits.forEach(edit => {
        if (undefined !== edit) {
            validEdits.push(edit);
        }
    })
    return validEdits;
};

const linesToSelections = (lines: line[]): Selection[] => {
    let cursorPos: Position | undefined = undefined;
    let selectionStart: Position | undefined = undefined;
    let pos: number = 0;

    lines.forEach((line, index) => {
        line.forEach(atom => {
            if ('black' === atom.face.fg) {
                // cursor pos
                cursorPos = new Position(index, pos);
            } else if ('white' === atom.face.fg) {
                // selection
                if (cursorPos) {
                    selectionStart = new Position(index, pos + atom.contents.length);
                } else {
                    selectionStart = new Position(index, pos);
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