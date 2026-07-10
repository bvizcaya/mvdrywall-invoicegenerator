/*
 * MV Drywall — shared invoice/estimate PDF builder.
 *
 * Used by both index.html (generate new) and history.html (re-download past).
 * Requires jsPDF to be loaded first.
 *
 * Expected `data` shape:
 *   {
 *     documentType: 'invoice' | 'estimate',
 *     invoiceNumber: '1042'            (optional; omitted for estimates)
 *     customer: { name, business, phone, address },
 *     address:  '5607 N 213th Ave, Elkhorn, NE 68022',
 *     invoiceDate: '2026-02-02',
 *     jobDate: '9/19/25',
 *     amount: 23350,
 *     comments: 'Two Story'
 *   }
 */

// Whole dollars with thousands commas, e.g. 23350 -> $23,350
function formatMoney(n) {
    return '$' + Number(n).toLocaleString('en-US', { maximumFractionDigits: 0 });
}

function formatInvoiceDate(value) {
    const d = new Date(value);
    if (isNaN(d)) return String(value);
    // Treat plain YYYY-MM-DD as a local date, not UTC (avoids off-by-one-day)
    const local = /^\d{4}-\d{2}-\d{2}$/.test(String(value))
        ? new Date(Number(value.slice(0, 4)), Number(value.slice(5, 7)) - 1, Number(value.slice(8, 10)))
        : d;
    return local.toLocaleDateString('en-US', { month: '2-digit', day: '2-digit', year: '2-digit' });
}

// Builds and returns a jsPDF document. Does not save it.
function buildInvoicePDF(data) {
    const { jsPDF } = window.jspdf;
    // Points + US Letter, matching the reference invoice exactly
    const doc = new jsPDF({ unit: 'pt', format: 'letter' });

    // Column x-positions (points). Option A: table spans the full page width.
    const L = 47;          // left text margin
    const X_JOB = 360;     // JOB DATE column
    const X_AMT = 470;     // AMOUNT column
    const RULE_L = 45;     // horizontal rule left
    const RULE_R = 565;    // horizontal rule right (mirrors the left margin)

    // Cell boundaries, derived so they always line up with the columns above
    const CELL_JOB_X = X_JOB - 8;
    const CELL_AMT_X = X_AMT - 8;

    // --- Header ---
    doc.setFont('helvetica', 'bold');
    doc.setFontSize(12);
    doc.text(String(data.documentType).toUpperCase(), L, 66);

    // Invoice number, right-aligned on the same line (invoices only)
    if (data.invoiceNumber) {
        doc.setFont('helvetica', 'bold');
        doc.setFontSize(11);
        doc.text('NO. ' + data.invoiceNumber, RULE_R, 66, { align: 'right' });
    }

    // Company name: reference uses Georgia (regular weight). jsPDF ships Times
    // as its built-in serif; swap to 'georgia' here if you embed the real font.
    doc.setFont('times', 'normal');
    doc.setFontSize(29);
    doc.text('MV Drywall Inc.', L, 110);

    doc.setFont('helvetica', 'normal');
    doc.setFontSize(11);
    doc.text('2307 Virginia St', L, 135);
    doc.text('Bellevue, NE 68147', L, 147);
    doc.text('(402)-981-9930', L, 165);

    const formattedDate = formatInvoiceDate(data.invoiceDate);
    doc.setFont('helvetica', 'bold');
    doc.setFontSize(12);
    doc.text('DATE: ' + formattedDate, L, 189);

    // BILL TO — serif in the reference
    doc.setFont('times', 'normal');
    doc.setFontSize(15);
    doc.text('BILL TO', L, 218);

    doc.setFont('helvetica', 'normal');
    doc.setFontSize(11);
    const billToName = data.customer.name || data.customer.business || '';
    doc.text(billToName, L, 238);

    if (data.customer.address) {
        const custLines = doc.splitTextToSize(data.customer.address, 260);
        custLines.forEach((ln, i) => doc.text(ln, L, 256 + i * 12));
    }

    // --- Table ---
    const Y_TOP_RULE = 290;
    const Y_HEAD = 314;
    const Y_MID_RULE = 320;    // thick (2pt) rule under the header row
    const Y_ROW = 345;
    const Y_TOTAL = 376;
    const Y_BOT_RULE = 380;

    doc.setDrawColor(0, 0, 0);
    doc.setLineWidth(1);
    doc.line(RULE_L, Y_TOP_RULE, RULE_R, Y_TOP_RULE);

    doc.setFont('helvetica', 'bold');
    doc.setFontSize(12);
    doc.text('HOUSE ADDRESS', L, Y_HEAD);
    doc.text('JOB DATE', X_JOB, Y_HEAD);
    doc.text('AMOUNT', X_AMT, Y_HEAD);

    // Shaded cells (drawn before text so the text sits on top).
    // Widths derive from the column positions, so they can't drift out of alignment.
    const ROW_H = 30;
    doc.setFillColor(217, 217, 217);
    doc.rect(RULE_L, Y_MID_RULE, CELL_JOB_X - RULE_L, ROW_H, 'F');   // house address cell
    doc.rect(CELL_AMT_X, Y_MID_RULE, RULE_R - CELL_AMT_X, ROW_H, 'F'); // amount cell
    doc.setFillColor(251, 252, 251);                                  // near-white job date column
    doc.rect(CELL_JOB_X, Y_MID_RULE, CELL_AMT_X - CELL_JOB_X, ROW_H, 'F');
    doc.rect(CELL_JOB_X, Y_MID_RULE + ROW_H, CELL_AMT_X - CELL_JOB_X, ROW_H, 'F');

    // 2pt rule across the full width, so all three cells share the same top edge
    doc.setLineWidth(2);
    doc.line(RULE_L - 1, Y_MID_RULE, RULE_R, Y_MID_RULE);

    doc.setTextColor(0, 0, 0);
    doc.setFont('helvetica', 'normal');
    doc.setFontSize(11);
    // Wrap within the address cell, leaving a little padding before the JOB DATE column
    const addrLines = doc.splitTextToSize(String(data.address), CELL_JOB_X - L - 10);
    doc.text(addrLines, L, Y_ROW);
    doc.text(String(data.jobDate), X_JOB, Y_ROW);

    doc.setFont('helvetica', 'bold');
    doc.text(formatMoney(data.amount), X_AMT, Y_ROW);

    doc.setFont('helvetica', 'normal');
    doc.text('TOTAL', X_JOB, Y_TOTAL);
    doc.setFont('helvetica', 'bold');
    doc.text(formatMoney(data.amount), X_AMT, Y_TOTAL);

    doc.setLineWidth(1);
    doc.line(RULE_L, Y_BOT_RULE, RULE_R, Y_BOT_RULE);

    // Comments: label bold, value regular (bold too when left as N/A)
    const comments = String(data.comments || 'N/A');
    const commentsIsDefault = comments.trim().toUpperCase() === 'N/A';
    doc.setFont('helvetica', 'bold');
    doc.setFontSize(11);
    doc.text('Comments:', L, 406);
    const labelW = doc.getTextWidth('Comments: ');
    doc.setFont('helvetica', commentsIsDefault ? 'bold' : 'normal');
    doc.text(comments, L + labelW, 406);

    // --- Footer ---
    // Anchors the bottom of the page and repeats the invoice number as a secondary
    // reference (the primary one stays in the header, where clients look for it).
    // Delete this block to remove the footer entirely.
    const Y_FOOT_RULE = 742;
    const Y_FOOT_TEXT = 756;
    doc.setDrawColor(191, 191, 191);
    doc.setLineWidth(0.5);
    doc.line(RULE_L, Y_FOOT_RULE, RULE_R, Y_FOOT_RULE);

    doc.setTextColor(115, 115, 115);
    doc.setFont('helvetica', 'normal');
    doc.setFontSize(9);
    const footLeft = data.invoiceNumber
        ? 'Invoice No. ' + data.invoiceNumber
        : String(data.documentType).charAt(0).toUpperCase() + String(data.documentType).slice(1);
    doc.text(footLeft, L, Y_FOOT_TEXT);
    doc.text('MV Drywall Inc.  \u00b7  (402)-981-9930', RULE_R, Y_FOOT_TEXT, { align: 'right' });

    // Reset colors so nothing leaks if this doc is reused
    doc.setTextColor(0, 0, 0);
    doc.setDrawColor(0, 0, 0);

    return doc;
}

function invoiceFileName(data) {
    const dateStr = formatInvoiceDate(data.invoiceDate).replace(/\//g, '-');
    const clientName = (data.customer.name || 'Client').replace(/\s+/g, '_');
    const typeName = String(data.documentType);
    const docTypeFile = typeName.charAt(0).toUpperCase() + typeName.slice(1);
    const num = data.invoiceNumber ? '_' + data.invoiceNumber : '';
    return `MV_Drywall_${docTypeFile}${num}_${clientName}_${dateStr}.pdf`;
}

// Builds and immediately downloads the PDF.
function downloadInvoicePDF(data) {
    const doc = buildInvoicePDF(data);
    doc.save(invoiceFileName(data));
}
