# Samples of OP-Z drum kits

These are bounces of the real OP-Z's drum kits. Each kit has 24 sounds, so there are two bounces per kit, one with the first 16 sounds and a second with the remaining 8, possibly padded with the second half of the bounce being silent. The bounces are 90BPM, with each sound played at it's natural pitch with wide-open envelope (min attack, full sustain and release) with no filtering, played in succession on a 4- or 8-step grid, which should leave plenty of room.

Note that only the first three drum tracks currently have samples... I will add the fourth in the next pr, once I have the slicing, trimming and import script made, and the support for sampled kits in the plugin code working using what we have here so far.

Note on trimming: So, if we make a rule to only split on zero-crossings, and then break the files up and look at the peak absolute value over the periods between every pair of zero crossings, as well as the duration of those periods (the fundamental frequency, in most cases, although the first peak in an autocorrelation would be the best way, strictly speaking, to find this), then it should be fairly easy to spot the onset points, especially if we weight the zero crossings by peak absolute amplitude between each one and the next crossing, and look for sharp increases, plus weight by proximity to the calculated divisions of the file based on it's length in samples and the expected position of the onsets, then it should be pretty easy to slice these exactly. we know that there is some amount of almost total silence at the tail of each sound, and we know that they should all have relatively sharp attacks, being percussion sounds in general, so those two features should be enough to locate the start and end of each sound almost perfectly, without even looking at pitch or period, just peak amplitude from each zero crossing to the next. Be careful about relying too much on autocorrelation with percussive sounds, as many of them include some flavor of noise component, which will confuse any method that relies heavily on autocorrelation/pitch estimates. Perhaps even just looking for silence below a certain noise floor for at least a certain minimum period, and then taking only regions which overlap a slightly negatively-offset calculated ideal beat grid based on the sample length, and then split on the ends of those regions, or nearest previous or next zero-crossing?

The files here are structured like this:

```
.
├── hat-kit1
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
├── hat-kit2
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
├── hat-kit3
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
├── hat-kit4
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
├── hat-kit5
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
├── kick-kit1
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
├── kick-kit2
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
├── kick-kit3
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
├── kick-kit4
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
├── snare-kit1
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
├── snare-kit2
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
├── snare-kit3
│   ├── hits-01-16.wav
│   └── hits-17-24.wav
└── snare-kit4
    ├── hits-01-16.wav
    └── hits-17-24.wav

14 directories, 26 files
```

