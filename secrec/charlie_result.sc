/*
 * Copyright (C) Cybernetica
 *
 * Research/Commercial License Usage
 * Licensees holding a valid Research License or Commercial License
 * for the Software may use this file according to the written
 * agreement between you and Cybernetica.
 *
 * GNU General Public License Usage
 * Alternatively, this file may be used under the terms of the GNU
 * General Public License version 3.0 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.  Please review the following information to
 * ensure the GNU General Public License version 3.0 requirements will be
 * met: http://www.gnu.org/copyleft/gpl-3.0.html.
 *
 * For further information, please contact us at sharemind@cyber.ee.
 */

import shared3p;
import keydb;
import shared3p_keydb;
import oblivious;

domain pd_shared3p shared3p;

void main() {
    keydb_connect("dbhost");

    pd_shared3p uint a = keydb_get("a");
    pd_shared3p uint b = keydb_get("b");

    pd_shared3p bool aliceWon = a > b;
    pd_shared3p uint winningBid = choose(aliceWon, a, b);

    publish("aliceWon", aliceWon);
    publish("winningBid", winningBid);

    keydb_disconnect();
}
