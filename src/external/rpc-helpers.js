
function getMode( rpcMsg ) {
    const msg = JSON.parse( rpcMsg );
    const params = msg.params || [];
    const modeLine = params[ 1 ];

    const insertModeLine = modeLine.filter( atom => atom.contents.includes( 'insert' ) );
    return insertModeLine.length > 0 ? 'insert' : 'normal'
}

function getMethod( rpcMsg ) {
    const msg = JSON.parse( rpcMsg );
    return msg.method || '';
}

function getLines( rpcMsg ) {
    const msg = JSON.parse( rpcMsg );
    const params = msg.params || [];
    return JSON.stringify( params[ 0 ] || [] );
}

module.exports = {
    getMethod,
    getMode,
    getLines,
};