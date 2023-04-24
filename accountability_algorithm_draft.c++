#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <iostream>
#include <blake3.h>
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace std;

// Best-Effort Broadcast
class BEB {
public:
    void broadcast(int pid, string message) {
        // broadcast message to all processes
        for (int i = 0; i < n; i++) {
            if (i != pid) {
                deliver(i, message);
            }
        }
    }

    virtual void deliver(int pid, string message) = 0;
};

class NS3BEB : public BEB {
public:
    void deliver(int pid, string message) override {

    }
};

class FITBEB : public BEB {
public:
    void deliver(int pid, string message) override {

    }
};

// Accountable Confirmer
class AC {
private:
        BEB& beb;
        int n;
        int t0;
        int pid;
        string value_i;
        bool confirmed_i;
        set<int> from_i;
        vector<string> light_certificate_i;
        vector<string> full_certificate_i;
        vector<string> obtained_light_certificates_i;
        vector<string> obtained_full_certificates_i;

        string share_sign_i(string v) {
            return sign(to_string(pid) + v);
        }

        bool share_verify(int pid, string v, string share) {
            return verify(to_string(pid) + v, share);
        }

        string sign(string v, string share) {
            return sign(v + share);
        }

        bool verify(string message, string signature) {
            return true;
        }

        bool is_valid_light_certificate(string v, string light_certificate_j) {
            vector<string> tokens;
            split(light_certificate_j, tokens);
            if (tokens[1] != v) {
                return false;
            }
            if (!verify_light_certificate(tokens[2])) {
                return false;
            }
            return true;
        }

        bool verify_light_certificate(string light_certificate) {
            return true;
        }

        string combine_light_certificates() {
            string result;
            for (string lc : light_certificate_i) {
                result += lc + ",";
            }
            return result.substr(0, result.size() - 1);
        }

        bool is_valid_full_certificate(string v, string full_certificate_j) {
            vector<string> tokens;
            split(full_certificate_j, tokens);
            if (tokens[1] != v) {
                return false;
            }
            if (!verify_full_certificate(tokens[2])) {
                return false;
            }
            return true;
        }

        bool verify_full_certificate(string full_certificate) {
            return true;
        }

        void split(string message, vector<string>& tokens) {
            string token;
            stringstream ss(message);
            while (getline(ss, token, ' ')) {
                tokens.push_back(token);
            }
        }

public:
    AC(BEB& beb, int n, int t0) : beb(beb), n(n), t0(t0) {}

    void init() {
        string value_i = "";
        char confirmed_i = false;
        from_i.clear();
        light_certificate_i.clear();
        full_certificate_i.clear();
        obtained_light_certificates_i.clear();
        obtained_full_certificates_i.clear();
    }

    void submit(string v) {
        value_i = v;
        string share = share_sign_i(v);
        string message = "submit " + v + " " + share;
        beb.broadcast(pid, message);
    }

    void trigger_confirm(string v) {
            cout << "Process " << pid << " confirmed value " << v << endl;
        }

    void trigger_abort() {
            cout << "Process " << pid << " aborted!" << endl;
        }

    void deliver(int pid, string message) {
        vector<string> tokens;
        split(message, tokens);
        string type = tokens[0];
        if (type == "submit") {
            string v = tokens[1];
            string share = tokens[2];
            if (share_verify(pid, v, share) && v == value_i && !from_i.count(pid)) {
                from_i.insert(pid);
                light_certificate_i.push_back(share);
                full_certificate_i.push_back("submit " + v + " " + share + " " + sign(v, share));
            }
        } else if (type == "light-certificate") {
            string v = tokens[1];
            string light_certificate_j = tokens[2];
            if (is_valid_light_certificate(v, light_certificate_j)) {
                obtained_light_certificates_i.push_back(message);
            }
        } else if (type == "full-certificate") {
            string v = tokens[1];
            string full_certificate_j = tokens[2];
            if (is_valid_full_certificate(v, full_certificate_j)) {
                obtained_full_certificates_i.push_back(message);
            }
        }
        if (from_i.size() >= n - t0 && !confirmed_i) {
            confirmed_i = true;
            trigger_confirm(value_i);
            string light_certificate = "light-certificate " + value_i + " " + combine_light_certificates();
            beb.broadcast(pid, light_certificate);
        }
        if (confirmed_i) {
            for (string light_certificate_j : obtained_light_certificates_i) {
                vector<string> tokens;
                split(light_certificate_j, tokens);
                string v = tokens[1];
                string light_certificate = tokens[2];
                if (is_valid_light_certificate(v, light_certificate)) {
                    obtained_light_certificates_i.push_back(light_certificate_j);
                }
            }
            for (int i = 0; i < obtained_light_certificates_i.size(); i++) {
                for (int j = i + 1; j < obtained_light_certificates_i.size(); j++) {
                    string certificate1 = obtained_light_certificates_i[i];
                    string certificate2 = obtained_light_certificates_i[j];
                    vector<string> tokens1, tokens2;
                    split(certificate1, tokens1);
                    split(certificate2, tokens2);
                    string v1 = tokens1[1], v2 = tokens2[1];
                    if (v1 == v2 && is_conflicting_light_certificate(tokens1[2], tokens2[2])) {
                        trigger_abort();
                    }
                }
            }
        }

    };

    int main() {
    // create a Best-Effort Broadcast instance
    NS3BEB beb;

    // create three Accountable Confirmer instances
    AC ac1(beb, 3, 1);
    AC ac2(beb, 3, 1);
    AC ac3(beb, 3, 1);

    // initialize each Accountable Confirmer instance
    ac1.init();
    ac2.init();
    ac3.init();

    // submit a value to the first Accountable Confirmer
    ac1.submit("hello");
    cout << "Value submitted to AC1: hello" << endl;

    // simulate the delivery of messages between the Accountable Confirmer instances
    beb.deliver(1, "submit hello abc123");
    cout << "AC1 received message: submit hello abc123" << endl;
    beb.deliver(2, "submit hello def456");
    cout << "AC2 received message: submit hello def456" << endl;
    beb.deliver(1, "light-certificate hello abc123,def456");
    cout << "AC1 received message: light-certificate hello abc123,def456" << endl;
    beb.deliver(2, "light-certificate hello abc123,def456");
    cout << "AC2 received message: light-certificate hello abc123,def456" << endl;
    beb.deliver(1, "full-certificate hello submit hello abc123 " + sign("hello", "abc123") + ",submit hello def456 " + sign("hello", "def456"));
    cout << "AC1 received message: full-certificate hello submit hello abc123 " + sign("hello", "abc123") + ",submit hello def456 " + sign("hello", "def456") << endl;

    // check if any confirmations have been triggered
    if (ac1.confirmed() || ac2.confirmed() || ac3.confirmed()) {
        cout << "A value has been confirmed." << endl;
    } else {
        cout << "No values have been confirmed yet." << endl;
    }

    return 0;
}


