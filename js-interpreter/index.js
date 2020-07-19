const CLIENT_ENDPOINT = 'https://icfpc2020-api.testkontur.ru/aliens/send?apiKey=b0a3d915b8d742a39897ab4dab931721'

const axios = require('axios').create({
    headers: { 'Content-Type': 'text/plain' },
    transformResponse: (res) => { return res },
    responseType: 'text'
})

//----------------------------------------------------------------------------
// Common Utilities
//----------------------------------------------------------------------------

class ParserResult {
    constructor(value, cursor){
        this.value  = value
        this.cursor = cursor
    }
}


//----------------------------------------------------------------------------
// AST Definitions
//----------------------------------------------------------------------------

class ObjectNode {
    constructor(obj){ this.cache = obj }
    async eval(){ return this.cache }
}

class NumberNode {
    constructor(x){ this.cache = x }
    async eval(){ return this.cache }
}

class ReferenceNode {
    constructor(key, slots){
        this.key   = key
        this.slots = slots
        this.cache = null
    }
    async eval(){
        if(this.cache === null){
            this.cache = await this.slots.get(this.key).eval()
        }
        return this.cache
    }
}

class ApplyNode {
    constructor(fn, arg){
        this.fn    = fn
        this.arg   = arg
        this.cache = null
    }
    async eval(){
        if(this.cache === null){
            const fn = await this.fn.eval()
            this.cache = await fn(this.arg)
        }
        return this.cache
    }
}

// Utility functions
const asNode = (obj) => { return new ObjectNode(obj) }


//----------------------------------------------------------------------------
// Instruction Definitions
//----------------------------------------------------------------------------

// #5: Successor
const IncDecl = async (x0) => { return await x0.eval() + 1 }
// #6: Predecessors
const DecDecl = async (x0) => { return await x0.eval() - 1 }
// #7: Sum
const AddDecl = async (x0) => { return async (x1) => { return await x0.eval() + await x1.eval() } }
// #9: Product
const MulDecl = async (x0) => { return async (x1) => { return await x0.eval() * await x1.eval() } }
// #10: Integer Division
const DivDecl = async (x0) => { return async (x1) => { return Math.trunc(await x0.eval() / await x1.eval()) } }

// #21: True (K Combinator)
const TrueDecl  = async (x0) => { return async () => { return await x0.eval() } }
// #22: False
const FalseDecl = async () => { return async (x1) => { return await x1.eval() } }

// #11: Equality and Booleans
const EqDecl = async (x0) => { return async (x1) => {
    const y0 = await x0.eval(), y1 = await x1.eval()
    return (y0 == y1) ? TrueDecl : FalseDecl
}}
// #12: Strict Less Than
const LtDecl = async (x0) => { return async (x1) => {
    const y0 = await x0.eval(), y1 = await x1.eval()
    return (y0 < y1) ? TrueDecl : FalseDecl
}}

// #16: Negate
const NegDecl = async (x0) => { return -(await x0.eval()) }
// #23: PowerOfTwo
const Pwr2Decl = async (x0) => { return Math.pow(2, await x0.eval()) }

// #18: S Combinator
const SDecl = async (x0) => { return async (x1) => { return async (x2) => {
    const t0 = await x0.eval()
    const t1 = await t0(x2)
    return await t1(new ApplyNode(x1, x2))
}}}
// #19: C Combinator
const CDecl = async (x0) => { return async (x1) => { return async (x2) => {
    const t0 = await x0.eval()
    const t1 = await t0(x2)
    return await t1(x1)
}}}
// #20: B Combinator
const BDecl = async (x0) => { return async (x1) => { return async (x2) => {
    const t0 = await x0.eval()
    return await t0(new ApplyNode(x1, x2))
}}}
// #24: I Combinator
const IDecl = async (x0) => { return await x0.eval() }

// #25: Cons
const ConsDecl = async (x0) => { return async (x1) => { return async (x2) => {
    const t0 = await x2.eval()
    const t1 = await t0(x0)
    return await t1(x1)
}}}
// #31: Vector
const VecDecl = ConsDecl
// #26: Car (First)
const CarDecl = async (x0) => { return (await x0.eval())(asNode(TrueDecl)) }
// #27: Cdr (Tail)
const CdrDecl = async (x0) => { return (await x0.eval())(asNode(FalseDecl)) }

// #28: Nil (Empty List)
const NilDecl = async () => { return TrueDecl }
// #29: Is Nil (Is Empty List)
const IsNilDecl = async (x0) => { return (await x0.eval()) === NilDecl ? TrueDecl : FalseDecl }

// #37: Is 0
const IfZeroDecl = async (x0) => { return (await x0.eval()) === 0 ? TrueDecl : FalseDecl }


// #13, 14: Modulate/Demodulate
class ModulatedValue {
    constructor(value){ this.value = value }
}

const ModDecl = async (x0) => {
    const recur = async (cur) => {
        if(typeof(cur) === 'number'){
            if(cur === 0){
                return '010'
            }else{
                const s = (cur > 0 ? '01' : '10')
                const t = Math.abs(cur).toString(2)
                const b = (t.length + 3) & ~3
                return s + ('1'.repeat(b / 4) + '0') + ('0'.repeat(b - t.length) + t)
            }
        }else if(cur === NilDecl){
            return '00'
        }else{
            const node = asNode(cur)
            return '11' + await recur(await CarDecl(node)) + await recur(await CdrDecl(node))
        }
    }
    return new ModulatedValue(await recur(await x0.eval()))
}

const DemDecl = async (x0) => {
    const s = (await x0.eval()).value
    const recur = async (i) => {
        if(s[i] != s[i + 1]){
            const sign = (s[i] == '0' ? 1 : -1)
            let b = 0, v = 0
            for(i += 2; s[i] != '0'; ++i){ b += 4 }
            ++i
            for(let j = 0; j < b; ++j, ++i){ v = (v * 2) + (s.charCodeAt(i) - 0x30) }
            return new ParserResult(sign * v, i)
        }else if(s[i] == '0' && s[i + 1] == '0'){
            return new ParserResult(NilDecl, i + 2)
        }else{
            const first = await recur(i + 2)
            const tail  = await recur(first.cursor)
            const t0 = await ConsDecl(asNode(first.value))
            const t1 = await t0(asNode(tail.value))
            return new ParserResult(t1, tail.cursor)
        }
    }
    return (await recur(0)).value
}

// #15: Send
const SendDecl = async (x0) => {
    const data = await x0.eval()
    const query = (await ModDecl(asNode(data))).value
    console.log('Send:', query)
    const response = await axios.post(CLIENT_ENDPOINT, query)
    console.log('Recv:', response.data)
    return await DemDecl(asNode(new ModulatedValue(response.data)))
}

// #32: Draw
class Picture {
    constructor(){ this.coords = [] }
    push(x, y){ this.coords.push([x, y]) }
}

const DrawDecl = async (x0, interpreter) => {
    const picture = new Picture()
    let cur = await x0.eval()
    while(cur !== NilDecl){
        const node  = asNode(cur)
        const first = await CarDecl(node)
        const tail  = await CdrDecl(node)
        const vec   = asNode(first)
        picture.push(await CarDecl(vec), await CdrDecl(vec))
        cur = tail
    }
    interpreter.screens.push(picture)
    return picture
}

// #34: Multiple Draw
const MultipleDrawDecl = async (x0, interpreter) => {
    const cur = await x0.eval()
    if(cur === NilDecl){ return NilDecl }
    const node  = asNode(cur)
    const first = await CarDecl(node)
    const tail  = await CdrDecl(node)
    const pic   = await DrawDecl(asNode(first), interpreter)
    const t     = await ConsDecl(asNode(pic))
    return await t(asNode(await MultipleDrawDecl(asNode(tail), interpreter)))
}

// #38: Interact
const InteractDecl = async (protocol, interpreter) => { return async (state) => { return async (vector) => {
    const p   = await protocol.eval()
    const t   = await p(state)
    const fsd = await t(vector)
    const fsdNode  = asNode(fsd)
    const flag     = await CarDecl(fsdNode)
    const sdNode   = asNode(await CdrDecl(fsdNode))
    const newState = await CarDecl(sdNode)
    const dNode    = asNode(await CdrDecl(sdNode))
    const data     = await CarDecl(dNode)
    if(flag === 0){
        const r0 = await ConsDecl(asNode(newState))
        const r1 = await MultipleDrawDecl(asNode(data), interpreter)
        const r2 = await ConsDecl(asNode(r1))
        const r3 = await r2(asNode(NilDecl))
        return await r0(asNode(r3))
    }else{
        const r0 = await InteractDecl(protocol, interpreter)
        const r1 = await r0(asNode(newState))
        const r2 = await SendDecl(asNode(data))
        return await r1(asNode(r2))
    }
}}}

// Debugging utilities
const isListObject = async (x) => {
    if(typeof(x) === 'number'){ return false }
    if(x instanceof ModulatedValue){ return false }
    if(x instanceof Picture){ return false }
    if(x === NilDecl){ return true }
    return await isListObject(await CdrDecl(asNode(x)))
}

const stringifyObject = async (x) => {
    if(typeof(x) === 'number'){
        return x.toString()
    }else if(x instanceof ModulatedValue){
        return '[' + x.value + ']'
    }else if(x instanceof Picture){
        return 'Picture'
    }else if(x === NilDecl){
        return 'nil'
    }else if(x === TrueDecl){
        return 't'
    }else if(x === FalseDecl){
        return 'f'
    }else if(await isListObject(x)){
        let cur = x, result = '(', isFirst = true
        while(cur !== NilDecl){
            if(!isFirst){ result += ' ,' }
            isFirst = false
            const node  = asNode(cur)
            const first = await CarDecl(node)
            const tail  = await CdrDecl(node)
            result += ' ' + await stringifyObject(first)
            cur = tail
        }
        return result + ' )'
    }else{
        const node  = asNode(x)
        const first = await CarDecl(node)
        const tail  = await CdrDecl(node)
        return 'ap ap cons ' + await stringifyObject(first) + ' ' + await stringifyObject(tail)
    }
}


//----------------------------------------------------------------------------
// Interpreter
//----------------------------------------------------------------------------
class Interpreter {
    constructor(){
        this.screens = []
        this.slots = new Map()
        this.slots.set('inc',          asNode(IncDecl))
        this.slots.set('dec',          asNode(DecDecl))
        this.slots.set('add',          asNode(AddDecl))
        this.slots.set('mul',          asNode(MulDecl))
        this.slots.set('div',          asNode(DivDecl))
        this.slots.set('eq',           asNode(EqDecl))
        this.slots.set('lt',           asNode(LtDecl))
        this.slots.set('mod',          asNode(ModDecl))
        this.slots.set('dem',          asNode(DemDecl))
        this.slots.set('send',         asNode(SendDecl))
        this.slots.set('neg',          asNode(NegDecl))
        this.slots.set('s',            asNode(SDecl))
        this.slots.set('c',            asNode(CDecl))
        this.slots.set('b',            asNode(BDecl))
        this.slots.set('t',            asNode(TrueDecl))
        this.slots.set('f',            asNode(FalseDecl))
        this.slots.set('pwr2',         asNode(Pwr2Decl))
        this.slots.set('i',            asNode(IDecl))
        this.slots.set('cons',         asNode(ConsDecl))
        this.slots.set('car',          asNode(CarDecl))
        this.slots.set('cdr',          asNode(CdrDecl))
        this.slots.set('nil',          asNode(NilDecl))
        this.slots.set('isnil',        asNode(IsNilDecl))
        this.slots.set('vec',          asNode(VecDecl))
        this.slots.set('draw',         asNode(async (x0) => { return await DrawDecl(x0, this) }))
        this.slots.set('multipledraw', asNode(async (x0) => { return await MultipleDrawDecl(x0, this) }))
        this.slots.set('if0',          asNode(IfZeroDecl))
        this.slots.set('interact',     asNode(async (x0) => { return await InteractDecl(x0, this) }))
    }

    clearScreens(){
        this.screens = []
    }

    parse(line){
        const tokens = line.trim().split(/\s+/)
        const assigning = (tokens.length >= 2 && tokens[1] == '=')
        const recur = (i) => {
            const t = tokens[i]
            if(t == 'ap'){
                const x = recur(i + 1)
                const y = recur(x.cursor)
                return new ParserResult(new ApplyNode(x.value, y.value), y.cursor)
            }else if(t == '(' || t == ','){
                if(tokens[i + 1] == ')'){ return recur(i + 1) }
                const x  = recur(i + 1)
                const y  = recur(x.cursor)
                const t0 = new ApplyNode(new ReferenceNode('cons', this.slots), x.value)
                const t1 = new ApplyNode(t0, y.value)
                return new ParserResult(t1, y.cursor)
            }else if(t == ')'){
                return new ParserResult(new ReferenceNode('nil', this.slots), i + 1)
            }else if(t.match(/^-?\d+$/g)){
                return new ParserResult(new NumberNode(parseInt(t)), i + 1)
            }else{
                return new ParserResult(new ReferenceNode(t, this.slots), i + 1)
            }
        }
        const root = recur(assigning ? 2 : 0)
        if(assigning){ this.slots.set(tokens[0], root.value) }
        return root.value
    }

    async eval(line){
        return await this.parse(line).eval()
    }

    async interact(protocol, state, data){
        const script = 'ap ap ap interact ' + protocol + ' ' + state + ' ' + data
        const result = await this.parse(script).eval()
        const newState = await CarDecl(asNode(result))
        return await stringifyObject(newState)
    }
}

/*
const fs = require('fs')

const interpreter = new Interpreter()

fs.readFileSync('galaxy.txt', 'utf-8').split('\n').forEach((line) => {
    interpreter.parse(line)
})

process.stdin.resume()
process.stdin.setEncoding('utf8')

const reader = require('readline').createInterface({
    input:  process.stdin,
    output: process.stdout
})
reader.on('line', async (s) => {
    if(s.trim() !== ''){
        interpreter.clearScreens()
        const value = await interpreter.eval(s)
        console.log(value)
        console.log(await stringifyObject(value))
        console.log(interpreter.screens)
    }
})
*/

exports.Interpreter = Interpreter
