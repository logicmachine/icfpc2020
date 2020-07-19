const request = require('sync-request')
const BigInt = require('big-integer')

const CLIENT_ENDPOINT = 'https://icfpc2020-api.testkontur.ru/aliens/send?apiKey=b0a3d915b8d742a39897ab4dab931721'

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
    eval(){ return this.cache }
}

class NumberNode {
    constructor(x){ this.cache = x }
    eval(){ return this.cache }
}

class ReferenceNode {
    constructor(key, slots){
        this.key   = key
        this.slots = slots
        this.cache = null
    }
    eval(){
        if(this.cache === null){
            this.cache = this.slots.get(this.key).eval()
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
    eval(){
        if(this.cache === null){
            const fn = this.fn.eval()
            this.cache = fn(this.arg)
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
const IncDecl = (x0) => { return x0.eval().add(1) }
// #6: Predecessors
const DecDecl = (x0) => { return x0.eval().add(-1) }
// #7: Sum
const AddDecl = (x0) => { return (x1) => { return x0.eval().add(x1.eval()) } }
// #9: Product
const MulDecl = (x0) => { return (x1) => { return x0.eval().multiply(x1.eval()) } }
// #10: Integer Division
const DivDecl = (x0) => { return (x1) => { return x0.eval().divide(x1.eval()) } }

// #21: True (K Combinator)
const TrueDecl  = (x0) => { return () => { return x0.eval() } }
// #22: False
const FalseDecl = () => { return (x1) => { return x1.eval() } }

// #11: Equality and Booleans
const EqDecl = (x0) => { return (x1) => {
    const y0 = x0.eval(), y1 = x1.eval()
    return y0.equals(y1) ? TrueDecl : FalseDecl
}}
// #12: Strict Less Than
const LtDecl = (x0) => { return (x1) => {
    const y0 = x0.eval(), y1 = x1.eval()
    return (y0.compare(y1) < 0) ? TrueDecl : FalseDecl
}}

// #16: Negate
const NegDecl = (x0) => { return x0.eval().multiply(BigInt.minusOne) }
// #23: PowerOfTwo
const Pwr2Decl = (x0) => { return BigInt(2).pow(x0.eval()) }

// #18: S Combinator
const SDecl = (x0) => { return (x1) => { return (x2) => {
    const t0 = x0.eval()
    const t1 = t0(x2)
    return t1(new ApplyNode(x1, x2))
}}}
// #19: C Combinator
const CDecl = (x0) => { return (x1) => { return (x2) => {
    const t0 = x0.eval()
    const t1 = t0(x2)
    return t1(x1)
}}}
// #20: B Combinator
const BDecl = (x0) => { return (x1) => { return (x2) => {
    const t0 = x0.eval()
    return t0(new ApplyNode(x1, x2))
}}}
// #24: I Combinator
const IDecl = (x0) => { return x0.eval() }

// #25: Cons
const ConsDecl = (x0) => { return (x1) => { return (x2) => {
    const t0 = x2.eval()
    const t1 = t0(x0)
    return t1(x1)
}}}
// #31: Vector
const VecDecl = ConsDecl
// #26: Car (First)
const CarDecl = (x0) => { return (x0.eval())(asNode(TrueDecl)) }
// #27: Cdr (Tail)
const CdrDecl = (x0) => { return (x0.eval())(asNode(FalseDecl)) }

// #28: Nil (Empty List)
const NilDecl = () => { return TrueDecl }
// #29: Is Nil (Is Empty List)
const IsNilDecl = (x0) => { return (x0.eval()) === NilDecl ? TrueDecl : FalseDecl }

// #37: Is 0
const IfZeroDecl = (x0) => { return (x0.eval()) === 0 ? TrueDecl : FalseDecl }


// #13, 14: Modulate/Demodulate
class ModulatedValue {
    constructor(value){ this.value = value }
}

const ModDecl = (x0) => {
    const recur = (cur) => {
        if(cur instanceof BigInt){
            if(cur.equals(0)){
                return '010'
            }else{
                const s = (cur.compare(0) > 0 ? '01' : '10')
                const t = cur.abs().toString(2)
                const b = (t.length + 3) & ~3
                return s + ('1'.repeat(b / 4) + '0') + ('0'.repeat(b - t.length) + t)
            }
        }else if(cur === NilDecl){
            return '00'
        }else{
            const node = asNode(cur)
            return '11' + recur(CarDecl(node)) + recur(CdrDecl(node))
        }
    }
    return new ModulatedValue(recur(x0.eval()))
}

const DemDecl = (x0) => {
    const s = (x0.eval()).value
    const recur = (i) => {
        if(s[i] != s[i + 1]){
            const sign = (s[i] == '0' ? 1 : -1)
            let b = 0
            for(i += 2; s[i] != '0'; ++i){ b += 4 }
            ++i
            const v = (b == 0 ? BigInt.zero : BigInt(s.substr(i, b), 2))
            return new ParserResult(v.multiply(sign), i + b)
        }else if(s[i] == '0' && s[i + 1] == '0'){
            return new ParserResult(NilDecl, i + 2)
        }else{
            const first = recur(i + 2)
            const tail  = recur(first.cursor)
            const t0 = ConsDecl(asNode(first.value))
            const t1 = t0(asNode(tail.value))
            return new ParserResult(t1, tail.cursor)
        }
    }
    return (recur(0)).value
}

// #15: Send
const SendDecl = (x0) => {
    const data = x0.eval()
    const query = (ModDecl(asNode(data))).value
    console.log('Send:', query)
    const response = request('POST', CLIENT_ENDPOINT, { body: query }).getBody('utf8')
    console.log('Recv:', response)
    return DemDecl(asNode(new ModulatedValue(response)))
}

// #32: Draw
class Picture {
    constructor(){ this.coords = [] }
    push(x, y){ this.coords.push([x, y]) }
}

const DrawDecl = (x0, interpreter) => {
    const picture = new Picture()
    let cur = x0.eval()
    while(cur !== NilDecl){
        const node  = asNode(cur)
        const first = CarDecl(node)
        const tail  = CdrDecl(node)
        const vec   = asNode(first)
        picture.push(CarDecl(vec).valueOf(), CdrDecl(vec).valueOf())
        cur = tail
    }
    interpreter.screens.push(picture)
    return picture
}

// #34: Multiple Draw
const MultipleDrawDecl = (x0, interpreter) => {
    const cur = x0.eval()
    if(cur === NilDecl){ return NilDecl }
    const node  = asNode(cur)
    const first = CarDecl(node)
    const tail  = CdrDecl(node)
    const pic   = DrawDecl(asNode(first), interpreter)
    const t     = ConsDecl(asNode(pic))
    return t(asNode(MultipleDrawDecl(asNode(tail), interpreter)))
}

// #38: Interact
const InteractDecl = (protocol, interpreter) => { return (state) => { return (vector) => {
    const p   = protocol.eval()
    const t   = p(state)
    const fsd = t(vector)
    const fsdNode  = asNode(fsd)
    const flag     = CarDecl(fsdNode)
    const sdNode   = asNode(CdrDecl(fsdNode))
    const newState = CarDecl(sdNode)
    const dNode    = asNode(CdrDecl(sdNode))
    const data     = CarDecl(dNode)
    if(flag.equals(0)){
        const r0 = ConsDecl(asNode(newState))
        const r1 = MultipleDrawDecl(asNode(data), interpreter)
        const r2 = ConsDecl(asNode(r1))
        const r3 = r2(asNode(NilDecl))
        return r0(asNode(r3))
    }else{
        const r0 = InteractDecl(protocol, interpreter)
        const r1 = r0(asNode(newState))
        const r2 = SendDecl(asNode(data))
        return r1(asNode(r2))
    }
}}}

// Debugging utilities
const isListObject = (x) => {
    if(x instanceof BigInt){ return false }
    if(x instanceof ModulatedValue){ return false }
    if(x instanceof Picture){ return false }
    if(x === NilDecl){ return true }
    return isListObject(CdrDecl(asNode(x)))
}

const stringifyObject = (x) => {
    if(x instanceof BigInt){
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
    }else if(isListObject(x)){
        let cur = x, result = '(', isFirst = true
        while(cur !== NilDecl){
            if(!isFirst){ result += ' ,' }
            isFirst = false
            const node  = asNode(cur)
            const first = CarDecl(node)
            const tail  = CdrDecl(node)
            result += ' ' + stringifyObject(first)
            cur = tail
        }
        return result + ' )'
    }else{
        const node  = asNode(x)
        const first = CarDecl(node)
        const tail  = CdrDecl(node)
        return 'ap ap cons ' + stringifyObject(first) + ' ' + stringifyObject(tail)
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
        this.slots.set('draw',         asNode((x0) => { return DrawDecl(x0, this) }))
        this.slots.set('multipledraw', asNode((x0) => { return MultipleDrawDecl(x0, this) }))
        this.slots.set('if0',          asNode(IfZeroDecl))
        this.slots.set('interact',     asNode((x0) => { return InteractDecl(x0, this) }))
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
                return new ParserResult(new NumberNode(BigInt(t)), i + 1)
            }else{
                return new ParserResult(new ReferenceNode(t, this.slots), i + 1)
            }
        }
        const root = recur(assigning ? 2 : 0)
        if(assigning){ this.slots.set(tokens[0], root.value) }
        return root.value
    }

    eval(line){
        return this.parse(line).eval()
    }

    interact(protocol, state, data){
        const script = 'ap ap ap interact ' + protocol + ' ' + state + ' ' + data
        const result = this.parse(script).eval()
        const newState = CarDecl(asNode(result))
        return stringifyObject(newState)
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
reader.on('line', (s) => {
    if(s.trim() !== ''){
        interpreter.clearScreens()
        const value = interpreter.eval(s)
        console.log(value)
        console.log(stringifyObject(value))
        console.log(interpreter.screens)
    }
})
*/

exports.Interpreter = Interpreter
