<template>
  <div id="app">
    <nav class="navbar navbar-expand-md navbar-dark bg-dark fixed-top">
      <a href="#" class="navbar-brand">Galaxy Pad</a>
    </nav>
    <main role="main" class="container-fill">
      <div class="row">
        <div class="col-3">
          <div class="form-group">
            <label for="stateTextarea">State</label>
            <textarea id="stateTextarea" class="form-control" v-model="state"></textarea>
          </div>
          <div class="form-group">
            <label for="stateTextarea">Vector</label>
            <textarea id="stateTextarea" class="form-control" v-model="vector"></textarea>
          </div>
          <button class="btn btn-primary" @click="interact" :disabled="running">Interact</button>
        </div>
        <div class="col-9">
          <canvas ref="canvas" width="800" height="800" @click="clickScreen"></canvas>
        </div>
      </div>
    </main>
  </div>
</template>

<script>
import galaxy from './components/Galaxy.js'
import { Interpreter } from 'js-interpreter'

const interpreter = new Interpreter()
galaxy.forEach((line) => { interpreter.parse(line) })

const canvasSize  = 800
const canvasScale = 3
const palette = [
  'rgb( 31, 119, 180)',
  'rgb(255, 127,  14)',
  'rgb( 44, 160,  44)',
  'rgb(214,  39,  40)',
  'rgb(148, 103, 189)',
  'rgb(140,  86,  75)',
  'rgb(227, 119, 194)',
  'rgb(127, 127, 127)',
  'rgb(188, 189,  34)',
  'rgb( 23, 190, 207)'
]

export default {
  name: 'App',

  data(){
    return {
      state: 'nil',
      vector: 'ap ap cons 0 0',
      running: false
    }
  },

  methods: {
    async interact(){
      if(this.running){ return }
      interpreter.clearScreens()
      this.running = true
      this.state = await interpreter.interact('galaxy', this.state, this.vector)
      this.running = false
      this.ctx.fillStyle = 'black'
      this.ctx.fillRect(0, 0, canvasSize, canvasSize)
      const offset = Math.round(canvasSize / 2)
      for(let index = interpreter.screens.length - 1; index >= 0; --index){
        const pic = interpreter.screens[index]
        this.ctx.fillStyle = palette[index % palette.length]
        pic.coords.forEach((c) => {
          const x = c[0] * canvasScale + offset, y = c[1] * canvasScale + offset
          this.ctx.fillRect(x + 1, y + 1, canvasScale, canvasScale)
        })
      }
    },

    async clickScreen(e){
      if(this.running){ return }
      const offset = Math.round(canvasSize / 2)
      const rect  = e.target.getBoundingClientRect()
      const raw_x = e.clientX - rect.left
      const raw_y = e.clientY - rect.top
      const x = Math.floor((raw_x - offset) / canvasScale)
      const y = Math.floor((raw_y - offset) / canvasScale)
      this.vector = 'ap ap cons ' + x + ' ' + y
      this.interact()
    }
  },

  mounted(){
    this.ctx = this.$refs.canvas.getContext('2d')
  }
}
</script>

<style>
body {
  padding-top: 4rem;
}

.container-fill {
  margin: 0 30px;
}

#canvas {
  margin: 0;
  padding: 0;
  border: none;
}
</style>
