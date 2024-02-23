class Controller {
    constructor(machine) {
        this.machine = machine;
        this.state = {
            choice: null,
        };
    }

    updateState({ choice }) {
        this.state.choice = choice;

        this.machine.writeToCNC(this.sendGCODE);
    }

    printState() {
        console.log("BUTTON", this.state);
    }

    get sendGCODE() {
        let { choice } = this.state;

        return `${choice} \n`;
    }
}

module.exports = {
    Controller,
};

