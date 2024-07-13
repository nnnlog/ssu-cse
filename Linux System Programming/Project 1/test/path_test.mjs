import path from 'path';
import {execSync, spawnSync} from "child_process";
import random from 'random-int';

const __dirname = path.resolve();

const shuffle = (array) => {
    let currentIndex = array.length, randomIndex;

    // While there remain elements to shuffle.
    while (currentIndex != 0) {

        // Pick a remaining element.
        randomIndex = Math.floor(Math.random() * currentIndex);
        currentIndex--;

        // And swap it with the current element.
        [array[currentIndex], array[randomIndex]] = [
            array[randomIndex], array[currentIndex]];
    }

    return array;
};

const generateRandomString = (length) => {
    let result = '';
    const characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.';
    const charactersLength = characters.length;
    let counter = 0;
    while (counter < length) {
        result += characters.charAt(Math.floor(Math.random() * charactersLength));
        counter += 1;
    }
    return result;
};

const generateRandomPath = () => {
    if (!random(0, 50)) return "/";
    let depth = random(1, 100);
    let par = random(0, Math.floor(depth * 1.5));
    if (!random(0, 10)) par = 0;
    let self = random(0, 30);

    let arr = [];
    for (let i = 0; i < depth; i++) arr.push(generateRandomString(random(0, 20)));
    for (let i = 0; i < par; i++) arr.push("..");
    for (let i = 0; i < self; i++) arr.push(".");

    shuffle(arr);

    return `/${arr.join("/")}`;
};

const test = (pathname) => {
    let ans = path.normalize(pathname);
    while (ans.length > 1 && ans[ans.length - 1] === "/") ans = ans.slice(0, ans.length - 1);

    let out = spawnSync(`${__dirname}/path`, [pathname]).stdout.toString();
    if (ans === out) console.log(`Correct, ${pathname} : ${ans}`);
    else {
        console.log(`Fail, ${pathname} : expected "${ans}", got "${out}"`);
        process.exit(0);
    }
};

execSync(`gcc ${__dirname}/../file/path.c -c ${__dirname}/path.o`);
execSync(`gcc ${__dirname}/path_test.c ${__dirname}/path.o -o ${__dirname}/path_test`);

for (let i = 0; i < 1000; i++) {
    let t = generateRandomPath();
    test(t);
}

execSync(`rm ${__dirname}/*.o ${__dirname}/path_test`);
