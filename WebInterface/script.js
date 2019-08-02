axios.defaults.baseURL = /*'/api';*/'http://matrix/api';

Vue.component('animation-view', {
  template: `
<v-layout align-start wrap>
  <v-progress-linear v-if="uploadProgress" v-model="uploadProgress" :indeterminate="uploadProgress === 100" color="accent" class="mt-0 mb-4"></v-progress-linear>
  <v-progress-circular v-if="animationCount === 0" indeterminate color="primary"></v-progress-circular>

  <v-flex
    v-for="i in animationCount"
    :key="i"
    xs4
    sm3
    md2
    xl1
    d-flex
    child-flex
  >
    <v-card flat tile class="d-flex">
      <v-img
        class="black pixelated-image"
        :src="'http://matrix/api/animations/' + (i - 1) + '?dummy=' + imageReloadDummy"
        aspect-ratio="1"
      >
        <template v-slot:placeholder>
          <v-layout
            fill-height
            align-center
            justify-center
            ma-0
          >
            <v-progress-circular indeterminate color="primary"></v-progress-circular>
          </v-layout>
        </template>

        <v-layout justify-end>
          <v-btn
            @click="deleteAnimation(i - 1)"
            color="error"
            flat
            icon
          >
            <v-icon>close</v-icon>
          </v-btn>
        </v-layout>
      </v-img>
    </v-card>
  </v-flex>

  <v-btn
    @click="$refs.inputUpload.click()"
    color="accent"
    dark
    absolute
    bottom
    right
    fab
  >
    <v-icon>add</v-icon>
  </v-btn>
  <input v-show="false" ref="inputUpload" type="file" accept="image/gif" @change="uploadAnimation($event.target.files)" />

  <v-dialog
    v-model="deleteAnimationDialog"
    max-width="290"
  >
    <v-card>
      <v-card-title
        class="headline"
        primary-title
      >
        Gif Entfernen
      </v-card-title>

      <v-card-text>
        Soll diese Animation entfernt werden?
      </v-card-text>

      <v-card-actions>
        <v-spacer></v-spacer>
        <v-btn
          color="primary"
          flat
          @click="deleteAnimationDialog = false"
        >
          Abbrechen
        </v-btn>
        <v-btn
          color="error"
          flat
          @click="_deleteAnimation()"
        >
          Entfernen
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</v-layout>
  `,
  data: () => ({
    animationCount: 0,
    imageReloadDummy: 0,

    uploadProgress: 0,

    deleteAnimationDialog: false,
    deleteAnimationIndex: -1
  }),

  methods: {
    fetchAnimations: function () {
      axios.get('animations')
        .then(res => {
          this.animationCount = parseInt(res.data)
          this.imageReloadDummy += 1
        }).catch(err => console.error(err.response.data))
    },

    uploadAnimation: function (files) {
      if (this.uploadProgress > 0) return
      if (!files) return
      this.uploadProgress = 1

      let file = files[0]
      let formData = new FormData()
      formData.append('File', file)

      axios.post('animations', formData, {
        headers: {
          'Content-Type': 'multipart/form-data'
        },
        onUploadProgress: () => {
          this.uploadProgress = event.loaded / event.total * 100
        }
      })
        .then(res => this.fetchAnimations())
        .catch(err => console.error(err.response.data))
        .then(() => this.uploadProgress = 0)
    },

    deleteAnimation: function (index) {
      this.deleteAnimationIndex = index
      this.deleteAnimationDialog = true
    },

    _deleteAnimation: function () {
      this.deleteAnimationDialog = false

      axios.delete('animations/' + this.deleteAnimationIndex)
        .then(res => {
          this.fetchAnimations()
        }).catch(err => {
          console.error(err.response.data)
        })
    }
  },

  mounted: function () {
    this.fetchAnimations()
  }
})

Vue.component('control-bar', {
  template: `
<v-layout
  align-center
  justify-start
  wrap
>
  <v-flex xs12 sm3>
    <v-text-field
      type="number"
      v-model="cycleDelay"
      @change="setCycleDelay()"
      suffix="Sec."
      :min="cycleDelayMin"
      :max="cycleDelayMax"
      prepend-inner-icon="schedule"
      class="py-0 px-2 control-bar-input"
      full-width
      hide-details
      single-line
    ></v-text-field>
  </v-flex>

  <v-flex xs12 sm6>
    <v-layout
      justify-center
      align-center
      row
      wrap
    >
      <v-btn @click="prev" flat icon>
        <v-icon>fast_rewind</v-icon>
      </v-btn>
      <v-btn @click="togglePlaying" flat icon>
        <v-icon v-if="playing">pause</v-icon>
        <v-icon v-else>play_arrow</v-icon>
      </v-btn>
      <v-btn @click="next" flat icon>
        <v-icon>fast_forward</v-icon>
      </v-btn>
    </v-layout>
  </v-flex>
</v-layout>
  `,
  data: () => ({
    playing: false,

    cycleDelay: 10,
    cycleDelayMin: 1,
    cycleDelayMax: 600
  }),
  methods: {
    fetchCycleDelay: function () {
      // Fetch the current cycle delay
      axios.get('control/cycle')
        .then(res => {
          let delay = parseInt(res.data)

          // If the matrix' delay is 0, the animation is paused, otherwise it is playing
          this.playing = delay !== 0

          // Update the saved cycle delay if we are playing
          if (this.playing) this.cycleDelay = delay
        }).catch(err => console.error(err.response.data))
    },
    togglePlaying: function () {
      if (this.playing) {
        // We are currently playing. Stop the playback and send a cycle delay of 0 to pause
        this.playing = false
        axios.post('control/cycle', 0, { headers: { 'Content-Type': 'text/plain' } })
          .catch(err => console.error(err.response.data))
      } else {
        // We are currentplay paused. Resume the playback by sending the stored cycle delay
        this.playing = true
        axios.post('control/cycle', this.cycleDelay, { headers: { 'Content-Type': 'text/plain' } })
          .catch(err => console.error(err.response.data))
      }
    },
    setCycleDelay: function () {
      if (this.cycleDelay < this.cycleDelayMin) this.cycleDelay = this.cycleDelayMin
      if (this.cycleDelay > this.cycleDelayMax) this.cycleDelay = this.cycleDelayMax

      // Send the cycle delay, if we are playing back.
      if (this.playing) {
        axios.post('control/cycle', this.cycleDelay, { headers: { 'Content-Type': 'text/plain' } })
          .catch(err => console.error(err.response.data))
      }
    },
    next: function () {
      axios.post('control/next')
        .catch(err => console.error(err.response.data))
    },
    prev: function () {
      axios.post('control/prev')
        .catch(err => console.error(err.response.data))
    }
  },
  mounted: function () {
    this.fetchCycleDelay()
  }
})

const app = new Vue({
  el: '#app',
  vuetify: new Vuetify({
    theme: {
      themes: {
        light: {
          primary: '#30C5FF',
          secondary: '#2176AE',
          accent: '#D81E5B'
        }
      }
    }
  }),
  data: () => ({
    modeNames: ['Animation', 'Music'],
    currentMode: 0
  }),

  methods: {
    changeMode: function (mode) {
      this.currentMode = mode
    }
  }
})