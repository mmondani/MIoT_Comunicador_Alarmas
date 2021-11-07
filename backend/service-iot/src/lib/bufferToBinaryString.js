export const bufferToBinaryString = (buff) => {
    let result = []
    for (let b of buff) {
        result.push(b.toString(2).padStart(8, '0'))
    }

    return result.join('')
}