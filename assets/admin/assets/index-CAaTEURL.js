var ts=Object.defineProperty;var No=e=>{throw TypeError(e)};var rs=(e,t,r)=>t in e?ts(e,t,{enumerable:!0,configurable:!0,writable:!0,value:r}):e[t]=r;var yt=(e,t,r)=>rs(e,typeof t!="symbol"?t+"":t,r),Ba=(e,t,r)=>t.has(e)||No("Cannot "+r);var h=(e,t,r)=>(Ba(e,t,"read from private field"),r?r.call(e):t.get(e)),X=(e,t,r)=>t.has(e)?No("Cannot add the same private member more than once"):t instanceof WeakSet?t.add(e):t.set(e,r),re=(e,t,r,a)=>(Ba(e,t,"write to private field"),a?a.call(e,r):t.set(e,r),r),he=(e,t,r)=>(Ba(e,t,"access private method"),r);(function(){const t=document.createElement("link").relList;if(t&&t.supports&&t.supports("modulepreload"))return;for(const o of document.querySelectorAll('link[rel="modulepreload"]'))a(o);new MutationObserver(o=>{for(const s of o)if(s.type==="childList")for(const i of s.addedNodes)i.tagName==="LINK"&&i.rel==="modulepreload"&&a(i)}).observe(document,{childList:!0,subtree:!0});function r(o){const s={};return o.integrity&&(s.integrity=o.integrity),o.referrerPolicy&&(s.referrerPolicy=o.referrerPolicy),o.crossOrigin==="use-credentials"?s.credentials="include":o.crossOrigin==="anonymous"?s.credentials="omit":s.credentials="same-origin",s}function a(o){if(o.ep)return;o.ep=!0;const s=r(o);fetch(o.href,s)}})();const as="5";var Go;typeof window<"u"&&((Go=window.__svelte??(window.__svelte={})).v??(Go.v=new Set)).add(as);const os=1,ns=2,Xo=4,ss=8,is=16,ls=1,cs=2,Qo=4,ds=8,us=16,fs=1,vs=2,Le=Symbol(),en="http://www.w3.org/1999/xhtml",ps="http://www.w3.org/2000/svg",hs="@attach",_s=!1;var vo=Array.isArray,xs=Array.prototype.indexOf,Rr=Array.prototype.includes,Ca=Array.from,bs=Object.defineProperty,er=Object.getOwnPropertyDescriptor,tn=Object.getOwnPropertyDescriptors,ms=Object.prototype,gs=Array.prototype,po=Object.getPrototypeOf,To=Object.isExtensible;function qr(e){return typeof e=="function"}const gr=()=>{};function ys(e){return e()}function Ga(e){for(var t=0;t<e.length;t++)e[t]()}function rn(){var e,t,r=new Promise((a,o)=>{e=a,t=o});return{promise:r,resolve:e,reject:t}}function ho(e,t){if(Array.isArray(e))return e;if(!(Symbol.iterator in e))return Array.from(e);const r=[];for(const a of e)if(r.push(a),r.length===t)break;return r}const Ve=2,Dr=4,ca=8,_o=1<<24,It=16,St=32,rr=64,Ya=128,pt=512,Re=1024,Be=2048,Mt=4096,qe=8192,lt=16384,Er=32768,Po=1<<25,ar=65536,Za=1<<17,ws=1<<18,Vr=1<<19,an=1<<20,Pt=1<<25,kr=65536,Xa=1<<21,ea=1<<22,tr=1<<23,Bt=Symbol("$state"),on=Symbol("legacy props"),$s=Symbol(""),Rt=new class extends Error{constructor(){super(...arguments);yt(this,"name","StaleReactionError");yt(this,"message","The reaction that called `getAbortSignal()` was re-run or destroyed")}};var Yo;const xo=!!((Yo=globalThis.document)!=null&&Yo.contentType)&&globalThis.document.contentType.includes("xml");function ks(){throw new Error("https://svelte.dev/e/async_derived_orphan")}function Ss(e,t,r){throw new Error("https://svelte.dev/e/each_key_duplicate")}function Es(e){throw new Error("https://svelte.dev/e/effect_in_teardown")}function As(){throw new Error("https://svelte.dev/e/effect_in_unowned_derived")}function Ns(e){throw new Error("https://svelte.dev/e/effect_orphan")}function Ts(){throw new Error("https://svelte.dev/e/effect_update_depth_exceeded")}function Ps(e){throw new Error("https://svelte.dev/e/props_invalid_value")}function Is(){throw new Error("https://svelte.dev/e/state_descriptors_fixed")}function Ms(){throw new Error("https://svelte.dev/e/state_prototype_fixed")}function Os(){throw new Error("https://svelte.dev/e/state_unsafe_mutation")}function Cs(){throw new Error("https://svelte.dev/e/svelte_boundary_reset_onerror")}function Ls(){console.warn("https://svelte.dev/e/derived_inert")}function Rs(){console.warn("https://svelte.dev/e/select_multiple_invalid_value")}function Ds(){console.warn("https://svelte.dev/e/svelte_boundary_reset_noop")}function nn(e){return e===this.v}function zs(e,t){return e!=e?t==t:e!==t||e!==null&&typeof e=="object"||typeof e=="function"}function sn(e){return!zs(e,this.v)}let da=!1,js=!1;function Fs(){da=!0}let ze=null;function zr(e){ze=e}function Ke(e,t=!1,r){ze={p:ze,i:!1,c:null,e:null,s:e,x:null,r:te,l:da&&!t?{s:null,u:null,$:[]}:null}}function Ge(e){var t=ze,r=t.e;if(r!==null){t.e=null;for(var a of r)Nn(a)}return t.i=!0,ze=t.p,{}}function ua(){return!da||ze!==null&&ze.l===null}let fr=[];function ln(){var e=fr;fr=[],Ga(e)}function Wt(e){if(fr.length===0&&!Zr){var t=fr;queueMicrotask(()=>{t===fr&&ln()})}fr.push(e)}function Bs(){for(;fr.length>0;)ln()}function cn(e){var t=te;if(t===null)return ne.f|=tr,e;if((t.f&Er)===0&&(t.f&Dr)===0)throw e;Qt(e,t)}function Qt(e,t){for(;t!==null;){if((t.f&Ya)!==0){if((t.f&Er)===0)throw e;try{t.b.error(e);return}catch(r){e=r}}t=t.parent}throw e}const Ws=-7169;function Ee(e,t){e.f=e.f&Ws|t}function bo(e){(e.f&pt)!==0||e.deps===null?Ee(e,Re):Ee(e,Mt)}function dn(e){if(e!==null)for(const t of e)(t.f&Ve)===0||(t.f&kr)===0||(t.f^=kr,dn(t.deps))}function un(e,t,r){(e.f&Be)!==0?t.add(e):(e.f&Mt)!==0&&r.add(e),dn(e.deps),Ee(e,Re)}let ma=!1;function Vs(e){var t=ma;try{return ma=!1,[e(),ma]}finally{ma=t}}const cr=new Set;let U=null,Fe=null,Qa=null,Zr=!1,Wa=!1,Pr=null,ya=null;var Io=0;let Hs=1;var Ir,Mr,hr,Dt,At,aa,nt,oa,Zt,zt,Nt,Or,Cr,_r,Ie,wa,fn,$a,eo,ka,Us;const Ia=class Ia{constructor(){X(this,Ie);yt(this,"id",Hs++);yt(this,"current",new Map);yt(this,"previous",new Map);X(this,Ir,new Set);X(this,Mr,new Set);X(this,hr,new Set);X(this,Dt,new Map);X(this,At,new Map);X(this,aa,null);X(this,nt,[]);X(this,oa,[]);X(this,Zt,new Set);X(this,zt,new Set);X(this,Nt,new Map);X(this,Or,new Set);yt(this,"is_fork",!1);X(this,Cr,!1);X(this,_r,new Set)}skip_effect(t){h(this,Nt).has(t)||h(this,Nt).set(t,{d:[],m:[]}),h(this,Or).delete(t)}unskip_effect(t,r=a=>this.schedule(a)){var a=h(this,Nt).get(t);if(a){h(this,Nt).delete(t);for(var o of a.d)Ee(o,Be),r(o);for(o of a.m)Ee(o,Mt),r(o)}h(this,Or).add(t)}capture(t,r,a=!1){t.v!==Le&&!this.previous.has(t)&&this.previous.set(t,t.v),(t.f&tr)===0&&(this.current.set(t,[r,a]),Fe==null||Fe.set(t,r)),this.is_fork||(t.v=r)}activate(){U=this}deactivate(){U=null,Fe=null}flush(){try{Wa=!0,U=this,he(this,Ie,$a).call(this)}finally{Io=0,Qa=null,Pr=null,ya=null,Wa=!1,U=null,Fe=null,yr.clear()}}discard(){for(const t of h(this,Mr))t(this);h(this,Mr).clear(),h(this,hr).clear(),cr.delete(this)}register_created_effect(t){h(this,oa).push(t)}increment(t,r){let a=h(this,Dt).get(r)??0;if(h(this,Dt).set(r,a+1),t){let o=h(this,At).get(r)??0;h(this,At).set(r,o+1)}}decrement(t,r,a){let o=h(this,Dt).get(r)??0;if(o===1?h(this,Dt).delete(r):h(this,Dt).set(r,o-1),t){let s=h(this,At).get(r)??0;s===1?h(this,At).delete(r):h(this,At).set(r,s-1)}h(this,Cr)||a||(re(this,Cr,!0),Wt(()=>{re(this,Cr,!1),this.flush()}))}transfer_effects(t,r){for(const a of t)h(this,Zt).add(a);for(const a of r)h(this,zt).add(a);t.clear(),r.clear()}oncommit(t){h(this,Ir).add(t)}ondiscard(t){h(this,Mr).add(t)}on_fork_commit(t){h(this,hr).add(t)}run_fork_commit_callbacks(){for(const t of h(this,hr))t(this);h(this,hr).clear()}settled(){return(h(this,aa)??re(this,aa,rn())).promise}static ensure(){if(U===null){const t=U=new Ia;Wa||(cr.add(U),Zr||Wt(()=>{U===t&&t.flush()}))}return U}apply(){{Fe=null;return}}schedule(t){var o;if(Qa=t,(o=t.b)!=null&&o.is_pending&&(t.f&(Dr|ca|_o))!==0&&(t.f&Er)===0){t.b.defer_effect(t);return}for(var r=t;r.parent!==null;){r=r.parent;var a=r.f;if(Pr!==null&&r===te&&(ne===null||(ne.f&Ve)===0))return;if((a&(rr|St))!==0){if((a&Re)===0)return;r.f^=Re}}h(this,nt).push(r)}};Ir=new WeakMap,Mr=new WeakMap,hr=new WeakMap,Dt=new WeakMap,At=new WeakMap,aa=new WeakMap,nt=new WeakMap,oa=new WeakMap,Zt=new WeakMap,zt=new WeakMap,Nt=new WeakMap,Or=new WeakMap,Cr=new WeakMap,_r=new WeakMap,Ie=new WeakSet,wa=function(){return this.is_fork||h(this,At).size>0},fn=function(){for(const a of h(this,_r))for(const o of h(a,At).keys()){for(var t=!1,r=o;r.parent!==null;){if(h(this,Nt).has(r)){t=!0;break}r=r.parent}if(!t)return!0}return!1},$a=function(){var l,c;if(Io++>1e3&&(cr.delete(this),Js()),!he(this,Ie,wa).call(this)){for(const d of h(this,Zt))h(this,zt).delete(d),Ee(d,Be),this.schedule(d);for(const d of h(this,zt))Ee(d,Mt),this.schedule(d)}const t=h(this,nt);re(this,nt,[]),this.apply();var r=Pr=[],a=[],o=ya=[];for(const d of t)try{he(this,Ie,eo).call(this,d,r,a)}catch(f){throw hn(d),f}if(U=null,o.length>0){var s=Ia.ensure();for(const d of o)s.schedule(d)}if(Pr=null,ya=null,he(this,Ie,wa).call(this)||he(this,Ie,fn).call(this)){he(this,Ie,ka).call(this,a),he(this,Ie,ka).call(this,r);for(const[d,f]of h(this,Nt))pn(d,f)}else{h(this,Dt).size===0&&cr.delete(this),h(this,Zt).clear(),h(this,zt).clear();for(const d of h(this,Ir))d(this);h(this,Ir).clear(),Mo(a),Mo(r),(l=h(this,aa))==null||l.resolve()}var i=U;if(h(this,nt).length>0){const d=i??(i=this);h(d,nt).push(...h(this,nt).filter(f=>!h(d,nt).includes(f)))}i!==null&&(cr.add(i),he(c=i,Ie,$a).call(c))},eo=function(t,r,a){t.f^=Re;for(var o=t.first;o!==null;){var s=o.f,i=(s&(St|rr))!==0,l=i&&(s&Re)!==0,c=l||(s&qe)!==0||h(this,Nt).has(o);if(!c&&o.fn!==null){i?o.f^=Re:(s&Dr)!==0?r.push(o):ha(o)&&((s&It)!==0&&h(this,zt).add(o),Br(o));var d=o.first;if(d!==null){o=d;continue}}for(;o!==null;){var f=o.next;if(f!==null){o=f;break}o=o.parent}}},ka=function(t){for(var r=0;r<t.length;r+=1)un(t[r],h(this,Zt),h(this,zt))},Us=function(){var f,g,b;for(const y of cr){var t=y.id<this.id,r=[];for(const[p,[k,x]]of this.current){if(y.current.has(p)){var a=y.current.get(p)[0];if(t&&k!==a)y.current.set(p,[k,x]);else continue}r.push(p)}var o=[...y.current.keys()].filter(p=>!this.current.has(p));if(o.length===0)t&&y.discard();else if(r.length>0){if(t)for(const p of h(this,Or))y.unskip_effect(p,k=>{var x;(k.f&(It|ea))!==0?y.schedule(k):he(x=y,Ie,ka).call(x,[k])});y.activate();var s=new Set,i=new Map;for(var l of r)vn(l,o,s,i);i=new Map;var c=[...y.current.keys()].filter(p=>this.current.has(p)?this.current.get(p)[0]!==p:!0);for(const p of h(this,oa))(p.f&(lt|qe|Za))===0&&mo(p,c,i)&&((p.f&(ea|It))!==0?(Ee(p,Be),y.schedule(p)):h(y,Zt).add(p));if(h(y,nt).length>0){y.apply();for(var d of h(y,nt))he(f=y,Ie,eo).call(f,d,[],[]);re(y,nt,[])}y.deactivate()}}for(const y of cr)h(y,_r).has(this)&&(h(y,_r).delete(this),h(y,_r).size===0&&!he(g=y,Ie,wa).call(g)&&(y.activate(),he(b=y,Ie,$a).call(b)))};let Sr=Ia;function qs(e){var t=Zr;Zr=!0;try{for(var r;;){if(Bs(),U===null)return r;U.flush()}}finally{Zr=t}}function Js(){try{Ts()}catch(e){Qt(e,Qa)}}let wt=null;function Mo(e){var t=e.length;if(t!==0){for(var r=0;r<t;){var a=e[r++];if((a.f&(lt|qe))===0&&ha(a)&&(wt=new Set,Br(a),a.deps===null&&a.first===null&&a.nodes===null&&a.teardown===null&&a.ac===null&&On(a),(wt==null?void 0:wt.size)>0)){yr.clear();for(const o of wt){if((o.f&(lt|qe))!==0)continue;const s=[o];let i=o.parent;for(;i!==null;)wt.has(i)&&(wt.delete(i),s.push(i)),i=i.parent;for(let l=s.length-1;l>=0;l--){const c=s[l];(c.f&(lt|qe))===0&&Br(c)}}wt.clear()}}wt=null}}function vn(e,t,r,a){if(!r.has(e)&&(r.add(e),e.reactions!==null))for(const o of e.reactions){const s=o.f;(s&Ve)!==0?vn(o,t,r,a):(s&(ea|It))!==0&&(s&Be)===0&&mo(o,t,a)&&(Ee(o,Be),go(o))}}function mo(e,t,r){const a=r.get(e);if(a!==void 0)return a;if(e.deps!==null)for(const o of e.deps){if(Rr.call(t,o))return!0;if((o.f&Ve)!==0&&mo(o,t,r))return r.set(o,!0),!0}return r.set(e,!1),!1}function go(e){U.schedule(e)}function pn(e,t){if(!((e.f&St)!==0&&(e.f&Re)!==0)){(e.f&Be)!==0?t.d.push(e):(e.f&Mt)!==0&&t.m.push(e),Ee(e,Re);for(var r=e.first;r!==null;)pn(r,t),r=r.next}}function hn(e){Ee(e,Re);for(var t=e.first;t!==null;)hn(t),t=t.next}function Ks(e){let t=0,r=or(0),a;return()=>{$o()&&(n(r),Pn(()=>(t===0&&(a=Wr(()=>e(()=>Xr(r)))),t+=1,()=>{Wt(()=>{t-=1,t===0&&(a==null||a(),a=void 0,Xr(r))})})))}}var Gs=ar|Vr;function Ys(e,t,r,a){new Zs(e,t,r,a)}var ut,fo,ft,xr,Qe,vt,Ue,st,jt,br,Xt,Lr,na,sa,Ft,Ma,we,Xs,Qs,ei,to,Sa,Ea,ro,ao;class Zs{constructor(t,r,a,o){X(this,we);yt(this,"parent");yt(this,"is_pending",!1);yt(this,"transform_error");X(this,ut);X(this,fo,null);X(this,ft);X(this,xr);X(this,Qe);X(this,vt,null);X(this,Ue,null);X(this,st,null);X(this,jt,null);X(this,br,0);X(this,Xt,0);X(this,Lr,!1);X(this,na,new Set);X(this,sa,new Set);X(this,Ft,null);X(this,Ma,Ks(()=>(re(this,Ft,or(h(this,br))),()=>{re(this,Ft,null)})));var s;re(this,ut,t),re(this,ft,r),re(this,xr,i=>{var l=te;l.b=this,l.f|=Ya,a(i)}),this.parent=te.b,this.transform_error=o??((s=this.parent)==null?void 0:s.transform_error)??(i=>i),re(this,Qe,pa(()=>{he(this,we,to).call(this)},Gs))}defer_effect(t){un(t,h(this,na),h(this,sa))}is_rendered(){return!this.is_pending&&(!this.parent||this.parent.is_rendered())}has_pending_snippet(){return!!h(this,ft).pending}update_pending_count(t,r){he(this,we,ro).call(this,t,r),re(this,br,h(this,br)+t),!(!h(this,Ft)||h(this,Lr))&&(re(this,Lr,!0),Wt(()=>{re(this,Lr,!1),h(this,Ft)&&jr(h(this,Ft),h(this,br))}))}get_effect_pending(){return h(this,Ma).call(this),n(h(this,Ft))}error(t){if(!h(this,ft).onerror&&!h(this,ft).failed)throw t;U!=null&&U.is_fork?(h(this,vt)&&U.skip_effect(h(this,vt)),h(this,Ue)&&U.skip_effect(h(this,Ue)),h(this,st)&&U.skip_effect(h(this,st)),U.on_fork_commit(()=>{he(this,we,ao).call(this,t)})):he(this,we,ao).call(this,t)}}ut=new WeakMap,fo=new WeakMap,ft=new WeakMap,xr=new WeakMap,Qe=new WeakMap,vt=new WeakMap,Ue=new WeakMap,st=new WeakMap,jt=new WeakMap,br=new WeakMap,Xt=new WeakMap,Lr=new WeakMap,na=new WeakMap,sa=new WeakMap,Ft=new WeakMap,Ma=new WeakMap,we=new WeakSet,Xs=function(){try{re(this,vt,tt(()=>h(this,xr).call(this,h(this,ut))))}catch(t){this.error(t)}},Qs=function(t){const r=h(this,ft).failed;r&&re(this,st,tt(()=>{r(h(this,ut),()=>t,()=>()=>{})}))},ei=function(){const t=h(this,ft).pending;t&&(this.is_pending=!0,re(this,Ue,tt(()=>t(h(this,ut)))),Wt(()=>{var r=re(this,jt,document.createDocumentFragment()),a=Vt();r.append(a),re(this,vt,he(this,we,Ea).call(this,()=>tt(()=>h(this,xr).call(this,a)))),h(this,Xt)===0&&(h(this,ut).before(r),re(this,jt,null),wr(h(this,Ue),()=>{re(this,Ue,null)}),he(this,we,Sa).call(this,U))}))},to=function(){try{if(this.is_pending=this.has_pending_snippet(),re(this,Xt,0),re(this,br,0),re(this,vt,tt(()=>{h(this,xr).call(this,h(this,ut))})),h(this,Xt)>0){var t=re(this,jt,document.createDocumentFragment());Eo(h(this,vt),t);const r=h(this,ft).pending;re(this,Ue,tt(()=>r(h(this,ut))))}else he(this,we,Sa).call(this,U)}catch(r){this.error(r)}},Sa=function(t){this.is_pending=!1,t.transfer_effects(h(this,na),h(this,sa))},Ea=function(t){var r=te,a=ne,o=ze;xt(h(this,Qe)),_t(h(this,Qe)),zr(h(this,Qe).ctx);try{return Sr.ensure(),t()}catch(s){return cn(s),null}finally{xt(r),_t(a),zr(o)}},ro=function(t,r){var a;if(!this.has_pending_snippet()){this.parent&&he(a=this.parent,we,ro).call(a,t,r);return}re(this,Xt,h(this,Xt)+t),h(this,Xt)===0&&(he(this,we,Sa).call(this,r),h(this,Ue)&&wr(h(this,Ue),()=>{re(this,Ue,null)}),h(this,jt)&&(h(this,ut).before(h(this,jt)),re(this,jt,null)))},ao=function(t){h(this,vt)&&(We(h(this,vt)),re(this,vt,null)),h(this,Ue)&&(We(h(this,Ue)),re(this,Ue,null)),h(this,st)&&(We(h(this,st)),re(this,st,null));var r=h(this,ft).onerror;let a=h(this,ft).failed;var o=!1,s=!1;const i=()=>{if(o){Ds();return}o=!0,s&&Cs(),h(this,st)!==null&&wr(h(this,st),()=>{re(this,st,null)}),he(this,we,Ea).call(this,()=>{he(this,we,to).call(this)})},l=c=>{try{s=!0,r==null||r(c,i),s=!1}catch(d){Qt(d,h(this,Qe)&&h(this,Qe).parent)}a&&re(this,st,he(this,we,Ea).call(this,()=>{try{return tt(()=>{var d=te;d.b=this,d.f|=Ya,a(h(this,ut),()=>c,()=>i)})}catch(d){return Qt(d,h(this,Qe).parent),null}}))};Wt(()=>{var c;try{c=this.transform_error(t)}catch(d){Qt(d,h(this,Qe)&&h(this,Qe).parent);return}c!==null&&typeof c=="object"&&typeof c.then=="function"?c.then(l,d=>Qt(d,h(this,Qe)&&h(this,Qe).parent)):l(c)})};function _n(e,t,r,a){const o=ua()?fa:yo;var s=e.filter(b=>!b.settled);if(r.length===0&&s.length===0){a(t.map(o));return}var i=te,l=ti(),c=s.length===1?s[0].promise:s.length>1?Promise.all(s.map(b=>b.promise)):null;function d(b){l();try{a(b)}catch(y){(i.f&lt)===0&&Qt(y,i)}Na()}if(r.length===0){c.then(()=>d(t.map(o)));return}var f=xn();function g(){Promise.all(r.map(b=>ri(b))).then(b=>d([...t.map(o),...b])).catch(b=>Qt(b,i)).finally(()=>f())}c?c.then(()=>{l(),g(),Na()}):g()}function ti(){var e=te,t=ne,r=ze,a=U;return function(s=!0){xt(e),_t(t),zr(r),s&&(e.f&lt)===0&&(a==null||a.activate(),a==null||a.apply())}}function Na(e=!0){xt(null),_t(null),zr(null),e&&(U==null||U.deactivate())}function xn(){var e=te,t=e.b,r=U,a=t.is_rendered();return t.update_pending_count(1,r),r.increment(a,e),(o=!1)=>{t.update_pending_count(-1,r),r.decrement(a,e,o)}}function fa(e){var t=Ve|Be;return te!==null&&(te.f|=Vr),{ctx:ze,deps:null,effects:null,equals:nn,f:t,fn:e,reactions:null,rv:0,v:Le,wv:0,parent:te,ac:null}}function ri(e,t,r){let a=te;a===null&&ks();var o=void 0,s=or(Le),i=!ne,l=new Map;return _i(()=>{var y;var c=te,d=rn();o=d.promise;try{Promise.resolve(e()).then(d.resolve,d.reject).finally(Na)}catch(p){d.reject(p),Na()}var f=U;if(i){if((c.f&Er)!==0)var g=xn();if(a.b.is_rendered())(y=l.get(f))==null||y.reject(Rt),l.delete(f);else{for(const p of l.values())p.reject(Rt);l.clear()}l.set(f,d)}const b=(p,k=void 0)=>{if(g){var x=k===Rt;g(x)}if(!(k===Rt||(c.f&lt)!==0)){if(f.activate(),k)s.f|=tr,jr(s,k);else{(s.f&tr)!==0&&(s.f^=tr),jr(s,p);for(const[v,A]of l){if(l.delete(v),v===f)break;A.reject(Rt)}}f.deactivate()}};d.promise.then(b,p=>b(null,p||"unknown"))}),Ra(()=>{for(const c of l.values())c.reject(Rt)}),new Promise(c=>{function d(f){function g(){f===o?c(s):d(o)}f.then(g,g)}d(o)})}function pe(e){const t=fa(e);return Rn(t),t}function yo(e){const t=fa(e);return t.equals=sn,t}function ai(e){var t=e.effects;if(t!==null){e.effects=null;for(var r=0;r<t.length;r+=1)We(t[r])}}function wo(e){var t,r=te,a=e.parent;if(!Ht&&a!==null&&(a.f&(lt|qe))!==0)return Ls(),e.v;xt(a);try{e.f&=~kr,ai(e),t=Fn(e)}finally{xt(r)}return t}function bn(e){var t=wo(e);if(!e.equals(t)&&(e.wv=zn(),(!(U!=null&&U.is_fork)||e.deps===null)&&(U!==null?U.capture(e,t,!0):e.v=t,e.deps===null))){Ee(e,Re);return}Ht||(Fe!==null?($o()||U!=null&&U.is_fork)&&Fe.set(e,t):bo(e))}function oi(e){var t,r;if(e.effects!==null)for(const a of e.effects)(a.teardown||a.ac)&&((t=a.teardown)==null||t.call(a),(r=a.ac)==null||r.abort(Rt),a.teardown=gr,a.ac=null,ta(a,0),ko(a))}function mn(e){if(e.effects!==null)for(const t of e.effects)t.teardown&&Br(t)}let oo=new Set;const yr=new Map;let gn=!1;function or(e,t){var r={f:0,v:e,reactions:null,equals:nn,rv:0,wv:0};return r}function W(e,t){const r=or(e);return Rn(r),r}function ni(e,t=!1,r=!0){var o;const a=or(e);return t||(a.equals=sn),da&&r&&ze!==null&&ze.l!==null&&((o=ze.l).s??(o.s=[])).push(a),a}function S(e,t,r=!1){ne!==null&&(!kt||(ne.f&Za)!==0)&&ua()&&(ne.f&(Ve|It|ea|Za))!==0&&(ht===null||!Rr.call(ht,e))&&Os();let a=r?me(t):t;return jr(e,a,ya)}function jr(e,t,r=null){if(!e.equals(t)){yr.set(e,Ht?t:e.v);var a=Sr.ensure();if(a.capture(e,t),(e.f&Ve)!==0){const o=e;(e.f&Be)!==0&&wo(o),Fe===null&&bo(o)}e.wv=zn(),yn(e,Be,r),ua()&&te!==null&&(te.f&Re)!==0&&(te.f&(St|rr))===0&&(dt===null?mi([e]):dt.push(e)),!a.is_fork&&oo.size>0&&!gn&&si()}return t}function si(){gn=!1;for(const e of oo)(e.f&Re)!==0&&Ee(e,Mt),ha(e)&&Br(e);oo.clear()}function Oo(e,t=1){var r=n(e),a=t===1?r++:r--;return S(e,r),a}function Xr(e){S(e,e.v+1)}function yn(e,t,r){var a=e.reactions;if(a!==null)for(var o=ua(),s=a.length,i=0;i<s;i++){var l=a[i],c=l.f;if(!(!o&&l===te)){var d=(c&Be)===0;if(d&&Ee(l,t),(c&Ve)!==0){var f=l;Fe==null||Fe.delete(f),(c&kr)===0&&(c&pt&&(l.f|=kr),yn(f,Mt,r))}else if(d){var g=l;(c&It)!==0&&wt!==null&&wt.add(g),r!==null?r.push(g):go(g)}}}}function me(e){if(typeof e!="object"||e===null||Bt in e)return e;const t=po(e);if(t!==ms&&t!==gs)return e;var r=new Map,a=vo(e),o=W(0),s=$r,i=l=>{if($r===s)return l();var c=ne,d=$r;_t(null),zo(s);var f=l();return _t(c),zo(d),f};return a&&r.set("length",W(e.length)),new Proxy(e,{defineProperty(l,c,d){(!("value"in d)||d.configurable===!1||d.enumerable===!1||d.writable===!1)&&Is();var f=r.get(c);return f===void 0?i(()=>{var g=W(d.value);return r.set(c,g),g}):S(f,d.value,!0),!0},deleteProperty(l,c){var d=r.get(c);if(d===void 0){if(c in l){const f=i(()=>W(Le));r.set(c,f),Xr(o)}}else S(d,Le),Xr(o);return!0},get(l,c,d){var y;if(c===Bt)return e;var f=r.get(c),g=c in l;if(f===void 0&&(!g||(y=er(l,c))!=null&&y.writable)&&(f=i(()=>{var p=me(g?l[c]:Le),k=W(p);return k}),r.set(c,f)),f!==void 0){var b=n(f);return b===Le?void 0:b}return Reflect.get(l,c,d)},getOwnPropertyDescriptor(l,c){var d=Reflect.getOwnPropertyDescriptor(l,c);if(d&&"value"in d){var f=r.get(c);f&&(d.value=n(f))}else if(d===void 0){var g=r.get(c),b=g==null?void 0:g.v;if(g!==void 0&&b!==Le)return{enumerable:!0,configurable:!0,value:b,writable:!0}}return d},has(l,c){var b;if(c===Bt)return!0;var d=r.get(c),f=d!==void 0&&d.v!==Le||Reflect.has(l,c);if(d!==void 0||te!==null&&(!f||(b=er(l,c))!=null&&b.writable)){d===void 0&&(d=i(()=>{var y=f?me(l[c]):Le,p=W(y);return p}),r.set(c,d));var g=n(d);if(g===Le)return!1}return f},set(l,c,d,f){var C;var g=r.get(c),b=c in l;if(a&&c==="length")for(var y=d;y<g.v;y+=1){var p=r.get(y+"");p!==void 0?S(p,Le):y in l&&(p=i(()=>W(Le)),r.set(y+"",p))}if(g===void 0)(!b||(C=er(l,c))!=null&&C.writable)&&(g=i(()=>W(void 0)),S(g,me(d)),r.set(c,g));else{b=g.v!==Le;var k=i(()=>me(d));S(g,k)}var x=Reflect.getOwnPropertyDescriptor(l,c);if(x!=null&&x.set&&x.set.call(f,d),!b){if(a&&typeof c=="string"){var v=r.get("length"),A=Number(c);Number.isInteger(A)&&A>=v.v&&S(v,A+1)}Xr(o)}return!0},ownKeys(l){n(o);var c=Reflect.ownKeys(l).filter(g=>{var b=r.get(g);return b===void 0||b.v!==Le});for(var[d,f]of r)f.v!==Le&&!(d in l)&&c.push(d);return c},setPrototypeOf(){Ms()}})}function Co(e){try{if(e!==null&&typeof e=="object"&&Bt in e)return e[Bt]}catch{}return e}function ii(e,t){return Object.is(Co(e),Co(t))}var Lo,wn,$n,kn;function li(){if(Lo===void 0){Lo=window,wn=/Firefox/.test(navigator.userAgent);var e=Element.prototype,t=Node.prototype,r=Text.prototype;$n=er(t,"firstChild").get,kn=er(t,"nextSibling").get,To(e)&&(e.__click=void 0,e.__className=void 0,e.__attributes=null,e.__style=void 0,e.__e=void 0),To(r)&&(r.__t=void 0)}}function Vt(e=""){return document.createTextNode(e)}function Fr(e){return $n.call(e)}function va(e){return kn.call(e)}function u(e,t){return Fr(e)}function ie(e,t=!1){{var r=Fr(e);return r instanceof Comment&&r.data===""?va(r):r}}function _(e,t=1,r=!1){let a=e;for(;t--;)a=va(a);return a}function ci(e){e.textContent=""}function Sn(){return!1}function En(e,t,r){return document.createElementNS(t??en,e,void 0)}function di(e,t){if(t){const r=document.body;e.autofocus=!0,Wt(()=>{document.activeElement===r&&e.focus()})}}let Ro=!1;function ui(){Ro||(Ro=!0,document.addEventListener("reset",e=>{Promise.resolve().then(()=>{var t;if(!e.defaultPrevented)for(const r of e.target.elements)(t=r.__on_r)==null||t.call(r)})},{capture:!0}))}function La(e){var t=ne,r=te;_t(null),xt(null);try{return e()}finally{_t(t),xt(r)}}function fi(e,t,r,a=r){e.addEventListener(t,()=>La(r));const o=e.__on_r;o?e.__on_r=()=>{o(),a(!0)}:e.__on_r=()=>a(!0),ui()}function An(e){te===null&&(ne===null&&Ns(),As()),Ht&&Es()}function vi(e,t){var r=t.last;r===null?t.last=t.first=e:(r.next=e,e.prev=r,t.last=e)}function Et(e,t){var r=te;r!==null&&(r.f&qe)!==0&&(e|=qe);var a={ctx:ze,deps:null,nodes:null,f:e|Be|pt,first:null,fn:t,last:null,next:null,parent:r,b:r&&r.b,prev:null,teardown:null,wv:0,ac:null};U==null||U.register_created_effect(a);var o=a;if((e&Dr)!==0)Pr!==null?Pr.push(a):Sr.ensure().schedule(a);else if(t!==null){try{Br(a)}catch(i){throw We(a),i}o.deps===null&&o.teardown===null&&o.nodes===null&&o.first===o.last&&(o.f&Vr)===0&&(o=o.first,(e&It)!==0&&(e&ar)!==0&&o!==null&&(o.f|=ar))}if(o!==null&&(o.parent=r,r!==null&&vi(o,r),ne!==null&&(ne.f&Ve)!==0&&(e&rr)===0)){var s=ne;(s.effects??(s.effects=[])).push(o)}return a}function $o(){return ne!==null&&!kt}function Ra(e){const t=Et(ca,null);return Ee(t,Re),t.teardown=e,t}function Je(e){An();var t=te.f,r=!ne&&(t&St)!==0&&(t&Er)===0;if(r){var a=ze;(a.e??(a.e=[])).push(e)}else return Nn(e)}function Nn(e){return Et(Dr|an,e)}function pi(e){return An(),Et(ca|an,e)}function hi(e){Sr.ensure();const t=Et(rr|Vr,e);return(r={})=>new Promise(a=>{r.outro?wr(t,()=>{We(t),a(void 0)}):(We(t),a(void 0))})}function Tn(e){return Et(Dr,e)}function _i(e){return Et(ea|Vr,e)}function Pn(e,t=0){return Et(ca|t,e)}function F(e,t=[],r=[],a=[]){_n(a,t,r,o=>{Et(ca,()=>e(...o.map(n)))})}function pa(e,t=0){var r=Et(It|t,e);return r}function In(e,t=0){var r=Et(_o|t,e);return r}function tt(e){return Et(St|Vr,e)}function Mn(e){var t=e.teardown;if(t!==null){const r=Ht,a=ne;Do(!0),_t(null);try{t.call(null)}finally{Do(r),_t(a)}}}function ko(e,t=!1){var r=e.first;for(e.first=e.last=null;r!==null;){const o=r.ac;o!==null&&La(()=>{o.abort(Rt)});var a=r.next;(r.f&rr)!==0?r.parent=null:We(r,t),r=a}}function xi(e){for(var t=e.first;t!==null;){var r=t.next;(t.f&St)===0&&We(t),t=r}}function We(e,t=!0){var r=!1;(t||(e.f&ws)!==0)&&e.nodes!==null&&e.nodes.end!==null&&(bi(e.nodes.start,e.nodes.end),r=!0),Ee(e,Po),ko(e,t&&!r),ta(e,0);var a=e.nodes&&e.nodes.t;if(a!==null)for(const s of a)s.stop();Mn(e),e.f^=Po,e.f|=lt;var o=e.parent;o!==null&&o.first!==null&&On(e),e.next=e.prev=e.teardown=e.ctx=e.deps=e.fn=e.nodes=e.ac=e.b=null}function bi(e,t){for(;e!==null;){var r=e===t?null:va(e);e.remove(),e=r}}function On(e){var t=e.parent,r=e.prev,a=e.next;r!==null&&(r.next=a),a!==null&&(a.prev=r),t!==null&&(t.first===e&&(t.first=a),t.last===e&&(t.last=r))}function wr(e,t,r=!0){var a=[];Cn(e,a,!0);var o=()=>{r&&We(e),t&&t()},s=a.length;if(s>0){var i=()=>--s||o();for(var l of a)l.out(i)}else o()}function Cn(e,t,r){if((e.f&qe)===0){e.f^=qe;var a=e.nodes&&e.nodes.t;if(a!==null)for(const l of a)(l.is_global||r)&&t.push(l);for(var o=e.first;o!==null;){var s=o.next;if((o.f&rr)===0){var i=(o.f&ar)!==0||(o.f&St)!==0&&(e.f&It)!==0;Cn(o,t,i?r:!1)}o=s}}}function So(e){Ln(e,!0)}function Ln(e,t){if((e.f&qe)!==0){e.f^=qe,(e.f&Re)===0&&(Ee(e,Be),Sr.ensure().schedule(e));for(var r=e.first;r!==null;){var a=r.next,o=(r.f&ar)!==0||(r.f&St)!==0;Ln(r,o?t:!1),r=a}var s=e.nodes&&e.nodes.t;if(s!==null)for(const i of s)(i.is_global||t)&&i.in()}}function Eo(e,t){if(e.nodes)for(var r=e.nodes.start,a=e.nodes.end;r!==null;){var o=r===a?null:va(r);t.append(r),r=o}}let Aa=!1,Ht=!1;function Do(e){Ht=e}let ne=null,kt=!1;function _t(e){ne=e}let te=null;function xt(e){te=e}let ht=null;function Rn(e){ne!==null&&(ht===null?ht=[e]:ht.push(e))}let et=null,ot=0,dt=null;function mi(e){dt=e}let Dn=1,vr=0,$r=vr;function zo(e){$r=e}function zn(){return++Dn}function ha(e){var t=e.f;if((t&Be)!==0)return!0;if(t&Ve&&(e.f&=~kr),(t&Mt)!==0){for(var r=e.deps,a=r.length,o=0;o<a;o++){var s=r[o];if(ha(s)&&bn(s),s.wv>e.wv)return!0}(t&pt)!==0&&Fe===null&&Ee(e,Re)}return!1}function jn(e,t,r=!0){var a=e.reactions;if(a!==null&&!(ht!==null&&Rr.call(ht,e)))for(var o=0;o<a.length;o++){var s=a[o];(s.f&Ve)!==0?jn(s,t,!1):t===s&&(r?Ee(s,Be):(s.f&Re)!==0&&Ee(s,Mt),go(s))}}function Fn(e){var k;var t=et,r=ot,a=dt,o=ne,s=ht,i=ze,l=kt,c=$r,d=e.f;et=null,ot=0,dt=null,ne=(d&(St|rr))===0?e:null,ht=null,zr(e.ctx),kt=!1,$r=++vr,e.ac!==null&&(La(()=>{e.ac.abort(Rt)}),e.ac=null);try{e.f|=Xa;var f=e.fn,g=f();e.f|=Er;var b=e.deps,y=U==null?void 0:U.is_fork;if(et!==null){var p;if(y||ta(e,ot),b!==null&&ot>0)for(b.length=ot+et.length,p=0;p<et.length;p++)b[ot+p]=et[p];else e.deps=b=et;if($o()&&(e.f&pt)!==0)for(p=ot;p<b.length;p++)((k=b[p]).reactions??(k.reactions=[])).push(e)}else!y&&b!==null&&ot<b.length&&(ta(e,ot),b.length=ot);if(ua()&&dt!==null&&!kt&&b!==null&&(e.f&(Ve|Mt|Be))===0)for(p=0;p<dt.length;p++)jn(dt[p],e);if(o!==null&&o!==e){if(vr++,o.deps!==null)for(let x=0;x<r;x+=1)o.deps[x].rv=vr;if(t!==null)for(const x of t)x.rv=vr;dt!==null&&(a===null?a=dt:a.push(...dt))}return(e.f&tr)!==0&&(e.f^=tr),g}catch(x){return cn(x)}finally{e.f^=Xa,et=t,ot=r,dt=a,ne=o,ht=s,zr(i),kt=l,$r=c}}function gi(e,t){let r=t.reactions;if(r!==null){var a=xs.call(r,e);if(a!==-1){var o=r.length-1;o===0?r=t.reactions=null:(r[a]=r[o],r.pop())}}if(r===null&&(t.f&Ve)!==0&&(et===null||!Rr.call(et,t))){var s=t;(s.f&pt)!==0&&(s.f^=pt,s.f&=~kr),s.v!==Le&&bo(s),oi(s),ta(s,0)}}function ta(e,t){var r=e.deps;if(r!==null)for(var a=t;a<r.length;a++)gi(e,r[a])}function Br(e){var t=e.f;if((t&lt)===0){Ee(e,Re);var r=te,a=Aa;te=e,Aa=!0;try{(t&(It|_o))!==0?xi(e):ko(e),Mn(e);var o=Fn(e);e.teardown=typeof o=="function"?o:null,e.wv=Dn;var s;_s&&js&&(e.f&Be)!==0&&e.deps}finally{Aa=a,te=r}}}async function yi(){await Promise.resolve(),qs()}function n(e){var t=e.f,r=(t&Ve)!==0;if(ne!==null&&!kt){var a=te!==null&&(te.f&lt)!==0;if(!a&&(ht===null||!Rr.call(ht,e))){var o=ne.deps;if((ne.f&Xa)!==0)e.rv<vr&&(e.rv=vr,et===null&&o!==null&&o[ot]===e?ot++:et===null?et=[e]:et.push(e));else{(ne.deps??(ne.deps=[])).push(e);var s=e.reactions;s===null?e.reactions=[ne]:Rr.call(s,ne)||s.push(ne)}}}if(Ht&&yr.has(e))return yr.get(e);if(r){var i=e;if(Ht){var l=i.v;return((i.f&Re)===0&&i.reactions!==null||Wn(i))&&(l=wo(i)),yr.set(i,l),l}var c=(i.f&pt)===0&&!kt&&ne!==null&&(Aa||(ne.f&pt)!==0),d=(i.f&Er)===0;ha(i)&&(c&&(i.f|=pt),bn(i)),c&&!d&&(mn(i),Bn(i))}if(Fe!=null&&Fe.has(e))return Fe.get(e);if((e.f&tr)!==0)throw e.v;return e.v}function Bn(e){if(e.f|=pt,e.deps!==null)for(const t of e.deps)(t.reactions??(t.reactions=[])).push(e),(t.f&Ve)!==0&&(t.f&pt)===0&&(mn(t),Bn(t))}function Wn(e){if(e.v===Le)return!0;if(e.deps===null)return!1;for(const t of e.deps)if(yr.has(t)||(t.f&Ve)!==0&&Wn(t))return!0;return!1}function Wr(e){var t=kt;try{return kt=!0,e()}finally{kt=t}}function ur(e){if(!(typeof e!="object"||!e||e instanceof EventTarget)){if(Bt in e)no(e);else if(!Array.isArray(e))for(let t in e){const r=e[t];typeof r=="object"&&r&&Bt in r&&no(r)}}}function no(e,t=new Set){if(typeof e=="object"&&e!==null&&!(e instanceof EventTarget)&&!t.has(e)){t.add(e),e instanceof Date&&e.getTime();for(let a in e)try{no(e[a],t)}catch{}const r=po(e);if(r!==Object.prototype&&r!==Array.prototype&&r!==Map.prototype&&r!==Set.prototype&&r!==Date.prototype){const a=tn(r);for(let o in a){const s=a[o].get;if(s)try{s.call(e)}catch{}}}}}const pr=Symbol("events"),Vn=new Set,so=new Set;function Hn(e,t,r,a={}){function o(s){if(a.capture||io.call(t,s),!s.cancelBubble)return La(()=>r==null?void 0:r.call(this,s))}return e.startsWith("pointer")||e.startsWith("touch")||e==="wheel"?Wt(()=>{t.addEventListener(e,o,a)}):t.addEventListener(e,o,a),o}function Un(e,t,r,a,o){var s={capture:a,passive:o},i=Hn(e,t,r,s);(t===document.body||t===window||t===document||t instanceof HTMLMediaElement)&&Ra(()=>{t.removeEventListener(e,i,s)})}function Z(e,t,r){(t[pr]??(t[pr]={}))[e]=r}function Ut(e){for(var t=0;t<e.length;t++)Vn.add(e[t]);for(var r of so)r(e)}let jo=null;function io(e){var x,v;var t=this,r=t.ownerDocument,a=e.type,o=((x=e.composedPath)==null?void 0:x.call(e))||[],s=o[0]||e.target;jo=e;var i=0,l=jo===e&&e[pr];if(l){var c=o.indexOf(l);if(c!==-1&&(t===document||t===window)){e[pr]=t;return}var d=o.indexOf(t);if(d===-1)return;c<=d&&(i=c)}if(s=o[i]||e.target,s!==t){bs(e,"currentTarget",{configurable:!0,get(){return s||r}});var f=ne,g=te;_t(null),xt(null);try{for(var b,y=[];s!==null;){var p=s.assignedSlot||s.parentNode||s.host||null;try{var k=(v=s[pr])==null?void 0:v[a];k!=null&&(!s.disabled||e.target===s)&&k.call(s,e)}catch(A){b?y.push(A):b=A}if(e.cancelBubble||p===t||p===null)break;s=p}if(b){for(let A of y)queueMicrotask(()=>{throw A});throw b}}finally{e[pr]=t,delete e.currentTarget,_t(f),xt(g)}}}var Zo;const Va=((Zo=globalThis==null?void 0:globalThis.window)==null?void 0:Zo.trustedTypes)&&globalThis.window.trustedTypes.createPolicy("svelte-trusted-html",{createHTML:e=>e});function wi(e){return(Va==null?void 0:Va.createHTML(e))??e}function qn(e){var t=En("template");return t.innerHTML=wi(e.replaceAll("<!>","<!---->")),t.content}function ra(e,t){var r=te;r.nodes===null&&(r.nodes={start:e,end:t,a:null,t:null})}function w(e,t){var r=(t&fs)!==0,a=(t&vs)!==0,o,s=!e.startsWith("<!>");return()=>{o===void 0&&(o=qn(s?e:"<!>"+e),r||(o=Fr(o)));var i=a||wn?document.importNode(o,!0):o.cloneNode(!0);if(r){var l=Fr(i),c=i.lastChild;ra(l,c)}else ra(i,i);return i}}function $i(e,t,r="svg"){var a=!e.startsWith("<!>"),o=`<${r}>${a?e:"<!>"+e}</${r}>`,s;return()=>{if(!s){var i=qn(o),l=Fr(i);s=Fr(l)}var c=s.cloneNode(!0);return ra(c,c),c}}function ki(e,t){return $i(e,t,"svg")}function ue(){var e=document.createDocumentFragment(),t=document.createComment(""),r=Vt();return e.append(t,r),ra(t,r),e}function m(e,t){e!==null&&e.before(t)}function Si(e){return e.endsWith("capture")&&e!=="gotpointercapture"&&e!=="lostpointercapture"}const Ei=["beforeinput","click","change","dblclick","contextmenu","focusin","focusout","input","keydown","keyup","mousedown","mousemove","mouseout","mouseover","mouseup","pointerdown","pointermove","pointerout","pointerover","pointerup","touchend","touchmove","touchstart"];function Ai(e){return Ei.includes(e)}const Ni={formnovalidate:"formNoValidate",ismap:"isMap",nomodule:"noModule",playsinline:"playsInline",readonly:"readOnly",defaultvalue:"defaultValue",defaultchecked:"defaultChecked",srcobject:"srcObject",novalidate:"noValidate",allowfullscreen:"allowFullscreen",disablepictureinpicture:"disablePictureInPicture",disableremoteplayback:"disableRemotePlayback"};function Ti(e){return e=e.toLowerCase(),Ni[e]??e}const Pi=["touchstart","touchmove"];function Ii(e){return Pi.includes(e)}function P(e,t){var r=t==null?"":typeof t=="object"?`${t}`:t;r!==(e.__t??(e.__t=e.nodeValue))&&(e.__t=r,e.nodeValue=`${r}`)}function Mi(e,t){return Oi(e,t)}const ga=new Map;function Oi(e,{target:t,anchor:r,props:a={},events:o,context:s,intro:i=!0,transformError:l}){li();var c=void 0,d=hi(()=>{var f=r??t.appendChild(Vt());Ys(f,{pending:()=>{}},y=>{Ke({});var p=ze;s&&(p.c=s),o&&(a.$$events=o),c=e(y,a)||{},Ge()},l);var g=new Set,b=y=>{for(var p=0;p<y.length;p++){var k=y[p];if(!g.has(k)){g.add(k);var x=Ii(k);for(const C of[t,document]){var v=ga.get(C);v===void 0&&(v=new Map,ga.set(C,v));var A=v.get(k);A===void 0?(C.addEventListener(k,io,{passive:x}),v.set(k,1)):v.set(k,A+1)}}}};return b(Ca(Vn)),so.add(b),()=>{var x;for(var y of g)for(const v of[t,document]){var p=ga.get(v),k=p.get(y);--k==0?(v.removeEventListener(y,io),p.delete(y),p.size===0&&ga.delete(v)):p.set(y,k)}so.delete(b),f!==r&&((x=f.parentNode)==null||x.removeChild(f))}});return Ci.set(c,d),c}let Ci=new WeakMap;var $t,Tt,it,mr,ia,la,Oa;class Ao{constructor(t,r=!0){yt(this,"anchor");X(this,$t,new Map);X(this,Tt,new Map);X(this,it,new Map);X(this,mr,new Set);X(this,ia,!0);X(this,la,t=>{if(h(this,$t).has(t)){var r=h(this,$t).get(t),a=h(this,Tt).get(r);if(a)So(a),h(this,mr).delete(r);else{var o=h(this,it).get(r);o&&(h(this,Tt).set(r,o.effect),h(this,it).delete(r),o.fragment.lastChild.remove(),this.anchor.before(o.fragment),a=o.effect)}for(const[s,i]of h(this,$t)){if(h(this,$t).delete(s),s===t)break;const l=h(this,it).get(i);l&&(We(l.effect),h(this,it).delete(i))}for(const[s,i]of h(this,Tt)){if(s===r||h(this,mr).has(s))continue;const l=()=>{if(Array.from(h(this,$t).values()).includes(s)){var d=document.createDocumentFragment();Eo(i,d),d.append(Vt()),h(this,it).set(s,{effect:i,fragment:d})}else We(i);h(this,mr).delete(s),h(this,Tt).delete(s)};h(this,ia)||!a?(h(this,mr).add(s),wr(i,l,!1)):l()}}});X(this,Oa,t=>{h(this,$t).delete(t);const r=Array.from(h(this,$t).values());for(const[a,o]of h(this,it))r.includes(a)||(We(o.effect),h(this,it).delete(a))});this.anchor=t,re(this,ia,r)}ensure(t,r){var a=U,o=Sn();if(r&&!h(this,Tt).has(t)&&!h(this,it).has(t))if(o){var s=document.createDocumentFragment(),i=Vt();s.append(i),h(this,it).set(t,{effect:tt(()=>r(i)),fragment:s})}else h(this,Tt).set(t,tt(()=>r(this.anchor)));if(h(this,$t).set(a,t),o){for(const[l,c]of h(this,Tt))l===t?a.unskip_effect(c):a.skip_effect(c);for(const[l,c]of h(this,it))l===t?a.unskip_effect(c.effect):a.skip_effect(c.effect);a.oncommit(h(this,la)),a.ondiscard(h(this,Oa))}else h(this,la).call(this,a)}}$t=new WeakMap,Tt=new WeakMap,it=new WeakMap,mr=new WeakMap,ia=new WeakMap,la=new WeakMap,Oa=new WeakMap;function K(e,t,r=!1){var a=new Ao(e),o=r?ar:0;function s(i,l){a.ensure(i,l)}pa(()=>{var i=!1;t((l,c=0)=>{i=!0,s(c,l)}),i||s(-1,null)},o)}function ke(e,t){return t}function Li(e,t,r){for(var a=[],o=t.length,s,i=t.length,l=0;l<o;l++){let g=t[l];wr(g,()=>{if(s){if(s.pending.delete(g),s.done.add(g),s.pending.size===0){var b=e.outrogroups;lo(e,Ca(s.done)),b.delete(s),b.size===0&&(e.outrogroups=null)}}else i-=1},!1)}if(i===0){var c=a.length===0&&r!==null;if(c){var d=r,f=d.parentNode;ci(f),f.append(d),e.items.clear()}lo(e,t,!c)}else s={pending:new Set(t),done:new Set},(e.outrogroups??(e.outrogroups=new Set)).add(s)}function lo(e,t,r=!0){var a;if(e.pending.size>0){a=new Set;for(const i of e.pending.values())for(const l of i)a.add(e.items.get(l).e)}for(var o=0;o<t.length;o++){var s=t[o];if(a!=null&&a.has(s)){s.f|=Pt;const i=document.createDocumentFragment();Eo(s,i)}else We(t[o],r)}}var Fo;function Se(e,t,r,a,o,s=null){var i=e,l=new Map,c=(t&Xo)!==0;if(c){var d=e;i=d.appendChild(Vt())}var f=null,g=yo(()=>{var C=r();return vo(C)?C:C==null?[]:Ca(C)}),b,y=new Map,p=!0;function k(C){(A.effect.f&lt)===0&&(A.pending.delete(C),A.fallback=f,Ri(A,b,i,t,a),f!==null&&(b.length===0?(f.f&Pt)===0?So(f):(f.f^=Pt,Yr(f,null,i)):wr(f,()=>{f=null})))}function x(C){A.pending.delete(C)}var v=pa(()=>{b=n(g);for(var C=b.length,E=new Set,O=U,D=Sn(),T=0;T<C;T+=1){var z=b[T],I=a(z,T),H=p?null:l.get(I);H?(H.v&&jr(H.v,z),H.i&&jr(H.i,T),D&&O.unskip_effect(H.e)):(H=Di(l,p?i:Fo??(Fo=Vt()),z,I,T,o,t,r),p||(H.e.f|=Pt),l.set(I,H)),E.add(I)}if(C===0&&s&&!f&&(p?f=tt(()=>s(i)):(f=tt(()=>s(Fo??(Fo=Vt()))),f.f|=Pt)),C>E.size&&Ss(),!p)if(y.set(O,E),D){for(const[B,J]of l)E.has(B)||O.skip_effect(J.e);O.oncommit(k),O.ondiscard(x)}else k(O);n(g)}),A={effect:v,items:l,pending:y,outrogroups:null,fallback:f};p=!1}function Jr(e){for(;e!==null&&(e.f&St)===0;)e=e.next;return e}function Ri(e,t,r,a,o){var H,B,J,R,j,V,G,le,M;var s=(a&ss)!==0,i=t.length,l=e.items,c=Jr(e.effect.first),d,f=null,g,b=[],y=[],p,k,x,v;if(s)for(v=0;v<i;v+=1)p=t[v],k=o(p,v),x=l.get(k).e,(x.f&Pt)===0&&((B=(H=x.nodes)==null?void 0:H.a)==null||B.measure(),(g??(g=new Set)).add(x));for(v=0;v<i;v+=1){if(p=t[v],k=o(p,v),x=l.get(k).e,e.outrogroups!==null)for(const N of e.outrogroups)N.pending.delete(x),N.done.delete(x);if((x.f&qe)!==0&&(So(x),s&&((R=(J=x.nodes)==null?void 0:J.a)==null||R.unfix(),(g??(g=new Set)).delete(x))),(x.f&Pt)!==0)if(x.f^=Pt,x===c)Yr(x,null,r);else{var A=f?f.next:c;x===e.effect.last&&(e.effect.last=x.prev),x.prev&&(x.prev.next=x.next),x.next&&(x.next.prev=x.prev),Gt(e,f,x),Gt(e,x,A),Yr(x,A,r),f=x,b=[],y=[],c=Jr(f.next);continue}if(x!==c){if(d!==void 0&&d.has(x)){if(b.length<y.length){var C=y[0],E;f=C.prev;var O=b[0],D=b[b.length-1];for(E=0;E<b.length;E+=1)Yr(b[E],C,r);for(E=0;E<y.length;E+=1)d.delete(y[E]);Gt(e,O.prev,D.next),Gt(e,f,O),Gt(e,D,C),c=C,f=D,v-=1,b=[],y=[]}else d.delete(x),Yr(x,c,r),Gt(e,x.prev,x.next),Gt(e,x,f===null?e.effect.first:f.next),Gt(e,f,x),f=x;continue}for(b=[],y=[];c!==null&&c!==x;)(d??(d=new Set)).add(c),y.push(c),c=Jr(c.next);if(c===null)continue}(x.f&Pt)===0&&b.push(x),f=x,c=Jr(x.next)}if(e.outrogroups!==null){for(const N of e.outrogroups)N.pending.size===0&&(lo(e,Ca(N.done)),(j=e.outrogroups)==null||j.delete(N));e.outrogroups.size===0&&(e.outrogroups=null)}if(c!==null||d!==void 0){var T=[];if(d!==void 0)for(x of d)(x.f&qe)===0&&T.push(x);for(;c!==null;)(c.f&qe)===0&&c!==e.fallback&&T.push(c),c=Jr(c.next);var z=T.length;if(z>0){var I=(a&Xo)!==0&&i===0?r:null;if(s){for(v=0;v<z;v+=1)(G=(V=T[v].nodes)==null?void 0:V.a)==null||G.measure();for(v=0;v<z;v+=1)(M=(le=T[v].nodes)==null?void 0:le.a)==null||M.fix()}Li(e,T,I)}}s&&Wt(()=>{var N,Y;if(g!==void 0)for(x of g)(Y=(N=x.nodes)==null?void 0:N.a)==null||Y.apply()})}function Di(e,t,r,a,o,s,i,l){var c=(i&os)!==0?(i&is)===0?ni(r,!1,!1):or(r):null,d=(i&ns)!==0?or(o):null;return{v:c,i:d,e:tt(()=>(s(t,c??r,d??o,l),()=>{e.delete(a)}))}}function Yr(e,t,r){if(e.nodes)for(var a=e.nodes.start,o=e.nodes.end,s=t&&(t.f&Pt)===0?t.nodes.start:r;a!==null;){var i=va(a);if(s.before(a),a===o)return;a=i}}function Gt(e,t,r){t===null?e.effect.first=r:t.next=r,r===null?e.effect.last=t:r.prev=t}function $e(e,t,r,a,o){var l;var s=(l=t.$$slots)==null?void 0:l[r],i=!1;s===!0&&(s=t.children,i=!0),s===void 0||s(e,i?()=>a:a)}function zi(e,t,r){var a=new Ao(e);pa(()=>{var o=t()??null;a.ensure(o,o&&(s=>r(s,o)))},ar)}function ji(e,t,r,a,o,s){var i=null,l=e,c=new Ao(l,!1);pa(()=>{const d=t()||null;var f=ps;if(d===null){c.ensure(null,null);return}return c.ensure(d,g=>{if(d){if(i=En(d,f),ra(i,i),a){var b=i.appendChild(Vt());a(i,b)}te.nodes.end=i,g.before(i)}}),()=>{}},ar),Ra(()=>{})}function Fi(e,t){var r=void 0,a;In(()=>{r!==(r=t())&&(a&&(We(a),a=null),r&&(a=tt(()=>{Tn(()=>r(e))})))})}function Jn(e){var t,r,a="";if(typeof e=="string"||typeof e=="number")a+=e;else if(typeof e=="object")if(Array.isArray(e)){var o=e.length;for(t=0;t<o;t++)e[t]&&(r=Jn(e[t]))&&(a&&(a+=" "),a+=r)}else for(r in e)e[r]&&(a&&(a+=" "),a+=r);return a}function Bi(){for(var e,t,r=0,a="",o=arguments.length;r<o;r++)(e=arguments[r])&&(t=Jn(e))&&(a&&(a+=" "),a+=t);return a}function Wi(e){return typeof e=="object"?Bi(e):e??""}const Bo=[...` 	
\r\f \v\uFEFF`];function Vi(e,t,r){var a=e==null?"":""+e;if(r){for(var o of Object.keys(r))if(r[o])a=a?a+" "+o:o;else if(a.length)for(var s=o.length,i=0;(i=a.indexOf(o,i))>=0;){var l=i+s;(i===0||Bo.includes(a[i-1]))&&(l===a.length||Bo.includes(a[l]))?a=(i===0?"":a.substring(0,i))+a.substring(l+1):i=l}}return a===""?null:a}function Wo(e,t=!1){var r=t?" !important;":";",a="";for(var o of Object.keys(e)){var s=e[o];s!=null&&s!==""&&(a+=" "+o+": "+s+r)}return a}function Ha(e){return e[0]!=="-"||e[1]!=="-"?e.toLowerCase():e}function Hi(e,t){if(t){var r="",a,o;if(Array.isArray(t)?(a=t[0],o=t[1]):a=t,e){e=String(e).replaceAll(/\s*\/\*.*?\*\/\s*/g,"").trim();var s=!1,i=0,l=!1,c=[];a&&c.push(...Object.keys(a).map(Ha)),o&&c.push(...Object.keys(o).map(Ha));var d=0,f=-1;const k=e.length;for(var g=0;g<k;g++){var b=e[g];if(l?b==="/"&&e[g-1]==="*"&&(l=!1):s?s===b&&(s=!1):b==="/"&&e[g+1]==="*"?l=!0:b==='"'||b==="'"?s=b:b==="("?i++:b===")"&&i--,!l&&s===!1&&i===0){if(b===":"&&f===-1)f=g;else if(b===";"||g===k-1){if(f!==-1){var y=Ha(e.substring(d,f).trim());if(!c.includes(y)){b!==";"&&g++;var p=e.substring(d,g).trim();r+=" "+p+";"}}d=g+1,f=-1}}}}return a&&(r+=Wo(a)),o&&(r+=Wo(o,!0)),r=r.trim(),r===""?null:r}return e==null?null:String(e)}function Pe(e,t,r,a,o,s){var i=e.__className;if(i!==r||i===void 0){var l=Vi(r,a,s);l==null?e.removeAttribute("class"):t?e.className=l:e.setAttribute("class",l),e.__className=r}else if(s&&o!==s)for(var c in s){var d=!!s[c];(o==null||d!==!!o[c])&&e.classList.toggle(c,d)}return s}function Ua(e,t={},r,a){for(var o in r){var s=r[o];t[o]!==s&&(r[o]==null?e.style.removeProperty(o):e.style.setProperty(o,s,a))}}function Ui(e,t,r,a){var o=e.__style;if(o!==t){var s=Hi(t,a);s==null?e.removeAttribute("style"):e.style.cssText=s,e.__style=t}else a&&(Array.isArray(a)?(Ua(e,r==null?void 0:r[0],a[0]),Ua(e,r==null?void 0:r[1],a[1],"important")):Ua(e,r,a));return a}function co(e,t,r=!1){if(e.multiple){if(t==null)return;if(!vo(t))return Rs();for(var a of e.options)a.selected=t.includes(Vo(a));return}for(a of e.options){var o=Vo(a);if(ii(o,t)){a.selected=!0;return}}(!r||t!==void 0)&&(e.selectedIndex=-1)}function qi(e){var t=new MutationObserver(()=>{co(e,e.__value)});t.observe(e,{childList:!0,subtree:!0,attributes:!0,attributeFilter:["value"]}),Ra(()=>{t.disconnect()})}function Vo(e){return"__value"in e?e.__value:e.value}const Kr=Symbol("class"),Gr=Symbol("style"),Kn=Symbol("is custom element"),Gn=Symbol("is html"),Ji=xo?"option":"OPTION",Ki=xo?"select":"SELECT",Gi=xo?"progress":"PROGRESS";function dr(e,t){var r=Da(e);r.value===(r.value=t??void 0)||e.value===t&&(t!==0||e.nodeName!==Gi)||(e.value=t??"")}function qa(e,t){var r=Da(e);r.checked!==(r.checked=t??void 0)&&(e.checked=t)}function Yi(e,t){t?e.hasAttribute("selected")||e.setAttribute("selected",""):e.removeAttribute("selected")}function Ta(e,t,r,a){var o=Da(e);o[t]!==(o[t]=r)&&(t==="loading"&&(e[$s]=r),r==null?e.removeAttribute(t):typeof r!="string"&&Yn(e).includes(t)?e[t]=r:e.setAttribute(t,r))}function Zi(e,t,r,a,o=!1,s=!1){var i=Da(e),l=i[Kn],c=!i[Gn],d=t||{},f=e.nodeName===Ji;for(var g in t)g in r||(r[g]=null);r.class?r.class=Wi(r.class):r[Kr]&&(r.class=null),r[Gr]&&(r.style??(r.style=null));var b=Yn(e);for(const E in r){let O=r[E];if(f&&E==="value"&&O==null){e.value=e.__value="",d[E]=O;continue}if(E==="class"){var y=e.namespaceURI==="http://www.w3.org/1999/xhtml";Pe(e,y,O,a,t==null?void 0:t[Kr],r[Kr]),d[E]=O,d[Kr]=r[Kr];continue}if(E==="style"){Ui(e,O,t==null?void 0:t[Gr],r[Gr]),d[E]=O,d[Gr]=r[Gr];continue}var p=d[E];if(!(O===p&&!(O===void 0&&e.hasAttribute(E)))){d[E]=O;var k=E[0]+E[1];if(k!=="$$")if(k==="on"){const D={},T="$$"+E;let z=E.slice(2);var x=Ai(z);if(Si(z)&&(z=z.slice(0,-7),D.capture=!0),!x&&p){if(O!=null)continue;e.removeEventListener(z,d[T],D),d[T]=null}if(x)Z(z,e,O),Ut([z]);else if(O!=null){let I=function(H){d[E].call(this,H)};var C=I;d[T]=Hn(z,e,I,D)}}else if(E==="style")Ta(e,E,O);else if(E==="autofocus")di(e,!!O);else if(!l&&(E==="__value"||E==="value"&&O!=null))e.value=e.__value=O;else if(E==="selected"&&f)Yi(e,O);else{var v=E;c||(v=Ti(v));var A=v==="defaultValue"||v==="defaultChecked";if(O==null&&!l&&!A)if(i[E]=null,v==="value"||v==="checked"){let D=e;const T=t===void 0;if(v==="value"){let z=D.defaultValue;D.removeAttribute(v),D.defaultValue=z,D.value=D.__value=T?z:null}else{let z=D.defaultChecked;D.removeAttribute(v),D.defaultChecked=z,D.checked=T?z:!1}}else e.removeAttribute(E);else A||b.includes(v)&&(l||typeof O!="string")?(e[v]=O,v in i&&(i[v]=Le)):typeof O!="function"&&Ta(e,v,O)}}}return d}function Ho(e,t,r=[],a=[],o=[],s,i=!1,l=!1){_n(o,r,a,c=>{var d=void 0,f={},g=e.nodeName===Ki,b=!1;if(In(()=>{var p=t(...c.map(n)),k=Zi(e,d,p,s,i,l);b&&g&&"value"in p&&co(e,p.value);for(let v of Object.getOwnPropertySymbols(f))p[v]||We(f[v]);for(let v of Object.getOwnPropertySymbols(p)){var x=p[v];v.description===hs&&(!d||x!==d[v])&&(f[v]&&We(f[v]),f[v]=tt(()=>Fi(e,()=>x))),k[v]=x}d=k}),g){var y=e;Tn(()=>{co(y,d.value,!0),qi(y)})}b=!0})}function Da(e){return e.__attributes??(e.__attributes={[Kn]:e.nodeName.includes("-"),[Gn]:e.namespaceURI===en})}var Uo=new Map;function Yn(e){var t=e.getAttribute("is")||e.nodeName,r=Uo.get(t);if(r)return r;Uo.set(t,r=[]);for(var a,o=e,s=Element.prototype;s!==o;){a=tn(o);for(var i in a)a[i].set&&r.push(i);o=po(o)}return r}function nr(e,t,r=t){var a=new WeakSet;fi(e,"input",async o=>{var s=o?e.defaultValue:e.value;if(s=Ja(e)?Ka(s):s,r(s),U!==null&&a.add(U),await yi(),s!==(s=t())){var i=e.selectionStart,l=e.selectionEnd,c=e.value.length;if(e.value=s??"",l!==null){var d=e.value.length;i===l&&l===c&&d>c?(e.selectionStart=d,e.selectionEnd=d):(e.selectionStart=i,e.selectionEnd=Math.min(l,d))}}}),Wr(t)==null&&e.value&&(r(Ja(e)?Ka(e.value):e.value),U!==null&&a.add(U)),Pn(()=>{var o=t();if(e===document.activeElement){var s=U;if(a.has(s))return}Ja(e)&&o===Ka(e.value)||e.type==="date"&&!o&&!e.value||o!==e.value&&(e.value=o??"")})}function Ja(e){var t=e.type;return t==="number"||t==="range"}function Ka(e){return e===""?null:+e}function Xi(e=!1){const t=ze,r=t.l.u;if(!r)return;let a=()=>ur(t.s);if(e){let o=0,s={};const i=fa(()=>{let l=!1;const c=t.s;for(const d in c)c[d]!==s[d]&&(s[d]=c[d],l=!0);return l&&o++,o});a=()=>n(i)}r.b.length&&pi(()=>{qo(t,a),Ga(r.b)}),Je(()=>{const o=Wr(()=>r.m.map(ys));return()=>{for(const s of o)typeof s=="function"&&s()}}),r.a.length&&Je(()=>{qo(t,a),Ga(r.a)})}function qo(e,t){if(e.l.s)for(const r of e.l.s)n(r);t()}const Qi={get(e,t){if(!e.exclude.includes(t))return n(e.version),t in e.special?e.special[t]():e.props[t]},set(e,t,r){if(!(t in e.special)){var a=te;try{xt(e.parent_effect),e.special[t]=Yt({get[t](){return e.props[t]}},t,Qo)}finally{xt(a)}}return e.special[t](r),Oo(e.version),!0},getOwnPropertyDescriptor(e,t){if(!e.exclude.includes(t)&&t in e.props)return{enumerable:!0,configurable:!0,value:e.props[t]}},deleteProperty(e,t){return e.exclude.includes(t)||(e.exclude.push(t),Oo(e.version)),!0},has(e,t){return e.exclude.includes(t)?!1:t in e.props},ownKeys(e){return Reflect.ownKeys(e.props).filter(t=>!e.exclude.includes(t))}};function ge(e,t){return new Proxy({props:e,exclude:t,special:{},version:or(0),parent_effect:te},Qi)}const el={get(e,t){let r=e.props.length;for(;r--;){let a=e.props[r];if(qr(a)&&(a=a()),typeof a=="object"&&a!==null&&t in a)return a[t]}},set(e,t,r){let a=e.props.length;for(;a--;){let o=e.props[a];qr(o)&&(o=o());const s=er(o,t);if(s&&s.set)return s.set(r),!0}return!1},getOwnPropertyDescriptor(e,t){let r=e.props.length;for(;r--;){let a=e.props[r];if(qr(a)&&(a=a()),typeof a=="object"&&a!==null&&t in a){const o=er(a,t);return o&&!o.configurable&&(o.configurable=!0),o}}},has(e,t){if(t===Bt||t===on)return!1;for(let r of e.props)if(qr(r)&&(r=r()),r!=null&&t in r)return!0;return!1},ownKeys(e){const t=[];for(let r of e.props)if(qr(r)&&(r=r()),!!r){for(const a in r)t.includes(a)||t.push(a);for(const a of Object.getOwnPropertySymbols(r))t.includes(a)||t.push(a)}return t}};function Ae(...e){return new Proxy({props:e},el)}function Yt(e,t,r,a){var C;var o=!da||(r&cs)!==0,s=(r&ds)!==0,i=(r&us)!==0,l=a,c=!0,d=()=>(c&&(c=!1,l=i?Wr(a):a),l);let f;if(s){var g=Bt in e||on in e;f=((C=er(e,t))==null?void 0:C.set)??(g&&t in e?E=>e[t]=E:void 0)}var b,y=!1;s?[b,y]=Vs(()=>e[t]):b=e[t],b===void 0&&a!==void 0&&(b=d(),f&&(o&&Ps(),f(b)));var p;if(o?p=()=>{var E=e[t];return E===void 0?d():(c=!0,E)}:p=()=>{var E=e[t];return E!==void 0&&(l=void 0),E===void 0?l:E},o&&(r&Qo)===0)return p;if(f){var k=e.$$legacy;return(function(E,O){return arguments.length>0?((!o||!O||k||y)&&f(O?p():E),E):p()})}var x=!1,v=((r&ls)!==0?fa:yo)(()=>(x=!1,p()));s&&n(v);var A=te;return(function(E,O){if(arguments.length>0){const D=O?n(v):o&&s?me(E):E;return S(v,D),x=!0,l!==void 0&&(l=D),E}return Ht&&x||(A.f&lt)!==0?v.v:n(v)})}const tl="/aonx/v1";function Zn(){return localStorage.getItem("kagami_session_token")||""}async function _a(e,t,r=null,a={}){const o=a.token||Zn(),s={"Content-Type":"application/json"};o&&(s.Authorization=`Bearer ${o}`);const i=await fetch(tl+t,{method:e,headers:s,body:r?JSON.stringify(r):null});if(a.raw)return i;let l=null;const c=await i.text();if(c)try{l=JSON.parse(c)}catch{l=c}return{status:i.status,data:l}}const rt=(e,t)=>_a("GET",e,null,t),sr=(e,t,r)=>_a("POST",e,t,r),rl=(e,t,r)=>_a("PATCH",e,t,r);async function al(){const e=await fetch("/metrics");if(!e.ok)return new Map;const t=await e.text(),r=new Map;for(const a of t.split(`
`)){if(!a||a.startsWith("#"))continue;const o=a.match(/^([a-zA-Z_:][a-zA-Z0-9_:]*(?:\{[^}]*\})?)\s+([\d.eE+-]+(?:nan|inf)?)/i);o&&r.set(o[1],parseFloat(o[2]))}return r}const se=me({authToken:localStorage.getItem("kagami_auth_token")||"",sessionToken:localStorage.getItem("kagami_session_token")||"",username:localStorage.getItem("kagami_username")||"",acl:localStorage.getItem("kagami_acl")||"",loggedIn:!!localStorage.getItem("kagami_session_token"),logoutReason:""});function Pa(){localStorage.setItem("kagami_auth_token",se.authToken),localStorage.setItem("kagami_session_token",se.sessionToken),localStorage.setItem("kagami_username",se.username),localStorage.setItem("kagami_acl",se.acl)}async function ol(e,t){var s,i;const r=await sr("/auth/login",{username:e,password:t});if(r.status!==201)return{ok:!1,error:((s=r.data)==null?void 0:s.reason)||"Login failed"};const a=r.data.token,o=await sr("/session",{client_name:"kagami-admin",client_version:"1.0",hdid:"admin-dashboard-"+crypto.randomUUID().slice(0,8),auth:a});return o.status!==201?{ok:!1,error:((i=o.data)==null?void 0:i.reason)||"Session creation failed"}:(se.authToken=a,se.sessionToken=o.data.token,se.username=r.data.username,se.acl=r.data.acl,se.loggedIn=!0,Pa(),{ok:!0})}async function nl(){se.authToken&&await sr("/auth/logout",null,{token:se.authToken}),se.authToken="",se.sessionToken="",se.username="",se.acl="",se.loggedIn=!1,Pa()}async function sl(){if(!(!se.sessionToken||(await _a("PATCH","/session",null)).status===200)){if(se.authToken){const t=await sr("/session",{client_name:"kagami-admin",client_version:"1.0",hdid:"admin-dashboard-"+crypto.randomUUID().slice(0,8),auth:se.authToken});if(t.status===201){se.sessionToken=t.data.token,Pa();return}}se.authToken="",se.sessionToken="",se.username="",se.acl="",se.loggedIn=!1,se.logoutReason="Session expired. The server may have restarted.",Pa(),window.location.hash="#/login"}}let Qr=null;function il(){Qr||(Qr=setInterval(sl,6e4))}function Jo(){Qr&&(clearInterval(Qr),Qr=null)}Fs();/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 * 
 * Copyright (c) 2026 Lucide Icons and Contributors
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * ---
 * 
 * The following Lucide icons are derived from the Feather project:
 * 
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 * 
 * The MIT License (MIT) (for the icons listed above)
 * 
 * Copyright (c) 2013-present Cole Bemis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */const ll={xmlns:"http://www.w3.org/2000/svg",width:24,height:24,viewBox:"0 0 24 24",fill:"none",stroke:"currentColor","stroke-width":2,"stroke-linecap":"round","stroke-linejoin":"round"};/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 * 
 * Copyright (c) 2026 Lucide Icons and Contributors
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * ---
 * 
 * The following Lucide icons are derived from the Feather project:
 * 
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 * 
 * The MIT License (MIT) (for the icons listed above)
 * 
 * Copyright (c) 2013-present Cole Bemis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */const cl=e=>{for(const t in e)if(t.startsWith("aria-")||t==="role"||t==="title")return!0;return!1};/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 * 
 * Copyright (c) 2026 Lucide Icons and Contributors
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * ---
 * 
 * The following Lucide icons are derived from the Feather project:
 * 
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 * 
 * The MIT License (MIT) (for the icons listed above)
 * 
 * Copyright (c) 2013-present Cole Bemis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */const Ko=(...e)=>e.filter((t,r,a)=>!!t&&t.trim()!==""&&a.indexOf(t)===r).join(" ").trim();var dl=ki("<svg><!><!></svg>");function Ne(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]),a=ge(r,["name","color","size","strokeWidth","absoluteStrokeWidth","iconNode"]);Ke(t,!1);let o=Yt(t,"name",8,void 0),s=Yt(t,"color",8,"currentColor"),i=Yt(t,"size",8,24),l=Yt(t,"strokeWidth",8,2),c=Yt(t,"absoluteStrokeWidth",8,!1),d=Yt(t,"iconNode",24,()=>[]);Xi();var f=dl();Ho(f,(y,p,k)=>({...ll,...y,...a,width:i(),height:i(),stroke:s(),"stroke-width":p,class:k}),[()=>cl(a)?void 0:{"aria-hidden":"true"},()=>(ur(c()),ur(l()),ur(i()),Wr(()=>c()?Number(l())*24/Number(i()):l())),()=>(ur(Ko),ur(o()),ur(r),Wr(()=>Ko("lucide-icon","lucide",o()?`lucide-${o()}`:"",r.class)))]);var g=u(f);Se(g,1,d,ke,(y,p)=>{var k=pe(()=>ho(n(p),2));let x=()=>n(k)[0],v=()=>n(k)[1];var A=ue(),C=ie(A);ji(C,x,!0,(E,O)=>{Ho(E,()=>({...v()}))}),m(y,A)});var b=_(g);$e(b,t,"default",{}),m(e,f),Ge()}function ul(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["circle",{cx:"12",cy:"12",r:"10"}],["path",{d:"M4.929 4.929 19.07 19.071"}]];Ne(e,Ae({name:"ban"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function fl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M15 3h6v6"}],["path",{d:"M10 14 21 3"}],["path",{d:"M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"}]];Ne(e,Ae({name:"external-link"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function vl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M12 3q1 4 4 6.5t3 5.5a1 1 0 0 1-14 0 5 5 0 0 1 1-3 1 1 0 0 0 5 0c0-2-1.5-3-1.5-5q0-2 2.5-4"}]];Ne(e,Ae({name:"flame"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function pl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"m6 14 1.5-2.9A2 2 0 0 1 9.24 10H20a2 2 0 0 1 1.94 2.5l-1.54 6a2 2 0 0 1-1.95 1.5H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h3.9a2 2 0 0 1 1.69.9l.81 1.2a2 2 0 0 0 1.67.9H18a2 2 0 0 1 2 2v2"}]];Ne(e,Ae({name:"folder-open"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function Xn(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M2.586 17.414A2 2 0 0 0 2 18.828V21a1 1 0 0 0 1 1h3a1 1 0 0 0 1-1v-1a1 1 0 0 1 1-1h1a1 1 0 0 0 1-1v-1a1 1 0 0 1 1-1h.172a2 2 0 0 0 1.414-.586l.814-.814a6.5 6.5 0 1 0-4-4z"}],["circle",{cx:"16.5",cy:"7.5",r:".5",fill:"currentColor"}]];Ne(e,Ae({name:"key-round"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function hl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["rect",{width:"7",height:"9",x:"3",y:"3",rx:"1"}],["rect",{width:"7",height:"5",x:"14",y:"3",rx:"1"}],["rect",{width:"7",height:"9",x:"14",y:"12",rx:"1"}],["rect",{width:"7",height:"5",x:"3",y:"16",rx:"1"}]];Ne(e,Ae({name:"layout-dashboard"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function _l(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"m16 17 5-5-5-5"}],["path",{d:"M21 12H9"}],["path",{d:"M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4"}]];Ne(e,Ae({name:"log-out"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function xl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M14.106 5.553a2 2 0 0 0 1.788 0l3.659-1.83A1 1 0 0 1 21 4.619v12.764a1 1 0 0 1-.553.894l-4.553 2.277a2 2 0 0 1-1.788 0l-4.212-2.106a2 2 0 0 0-1.788 0l-3.659 1.83A1 1 0 0 1 3 19.381V6.618a1 1 0 0 1 .553-.894l4.553-2.277a2 2 0 0 1 1.788 0z"}],["path",{d:"M15 5.764v15"}],["path",{d:"M9 3.236v15"}]];Ne(e,Ae({name:"map"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function bl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M4 5h16"}],["path",{d:"M4 12h16"}],["path",{d:"M4 19h16"}]];Ne(e,Ae({name:"menu"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function ml(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M22 17a2 2 0 0 1-2 2H6.828a2 2 0 0 0-1.414.586l-2.202 2.202A.71.71 0 0 1 2 21.286V5a2 2 0 0 1 2-2h16a2 2 0 0 1 2 2z"}]];Ne(e,Ae({name:"message-square"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function gl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M20.985 12.486a9 9 0 1 1-9.473-9.472c.405-.022.617.46.402.803a6 6 0 0 0 8.268 8.268c.344-.215.825-.004.803.401"}]];Ne(e,Ae({name:"moon"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function Qn(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M5 12h14"}],["path",{d:"M12 5v14"}]];Ne(e,Ae({name:"plus"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function yl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M12 2v10"}],["path",{d:"M18.4 6.6a9 9 0 1 1-12.77.04"}]];Ne(e,Ae({name:"power"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function es(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M15.2 3a2 2 0 0 1 1.4.6l3.8 3.8a2 2 0 0 1 .6 1.4V19a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2z"}],["path",{d:"M17 21v-7a1 1 0 0 0-1-1H8a1 1 0 0 0-1 1v7"}],["path",{d:"M7 3v4a1 1 0 0 0 1 1h7"}]];Ne(e,Ae({name:"save"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function wl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"m21 21-4.34-4.34"}],["circle",{cx:"11",cy:"11",r:"8"}]];Ne(e,Ae({name:"search"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function $l(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M9.671 4.136a2.34 2.34 0 0 1 4.659 0 2.34 2.34 0 0 0 3.319 1.915 2.34 2.34 0 0 1 2.33 4.033 2.34 2.34 0 0 0 0 3.831 2.34 2.34 0 0 1-2.33 4.033 2.34 2.34 0 0 0-3.319 1.915 2.34 2.34 0 0 1-4.659 0 2.34 2.34 0 0 0-3.32-1.915 2.34 2.34 0 0 1-2.33-4.033 2.34 2.34 0 0 0 0-3.831A2.34 2.34 0 0 1 6.35 6.051a2.34 2.34 0 0 0 3.319-1.915"}],["circle",{cx:"12",cy:"12",r:"3"}]];Ne(e,Ae({name:"settings"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function kl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"}]];Ne(e,Ae({name:"shield"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function Sl(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["circle",{cx:"12",cy:"12",r:"4"}],["path",{d:"M12 2v2"}],["path",{d:"M12 20v2"}],["path",{d:"m4.93 4.93 1.41 1.41"}],["path",{d:"m17.66 17.66 1.41 1.41"}],["path",{d:"M2 12h2"}],["path",{d:"M20 12h2"}],["path",{d:"m6.34 17.66-1.41 1.41"}],["path",{d:"m19.07 4.93-1.41 1.41"}]];Ne(e,Ae({name:"sun"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function uo(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M10 11v6"}],["path",{d:"M14 11v6"}],["path",{d:"M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6"}],["path",{d:"M3 6h18"}],["path",{d:"M8 6V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"}]];Ne(e,Ae({name:"trash-2"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function El(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M16 21v-2a4 4 0 0 0-4-4H6a4 4 0 0 0-4 4v2"}],["path",{d:"M16 3.128a4 4 0 0 1 0 7.744"}],["path",{d:"M22 21v-2a4 4 0 0 0-3-3.87"}],["circle",{cx:"9",cy:"7",r:"4"}]];Ne(e,Ae({name:"users"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}function Al(e,t){const r=ge(t,["children","$$slots","$$events","$$legacy"]);/**
 * @license lucide-svelte v1.0.1 - ISC
 *
 * ISC License
 *
 * Copyright (c) 2026 Lucide Icons and Contributors
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ---
 *
 * The following Lucide icons are derived from the Feather project:
 *
 * airplay, alert-circle, alert-octagon, alert-triangle, aperture, arrow-down-circle, arrow-down-left, arrow-down-right, arrow-down, arrow-left-circle, arrow-left, arrow-right-circle, arrow-right, arrow-up-circle, arrow-up-left, arrow-up-right, arrow-up, at-sign, calendar, cast, check, chevron-down, chevron-left, chevron-right, chevron-up, chevrons-down, chevrons-left, chevrons-right, chevrons-up, circle, clipboard, clock, code, columns, command, compass, corner-down-left, corner-down-right, corner-left-down, corner-left-up, corner-right-down, corner-right-up, corner-up-left, corner-up-right, crosshair, database, divide-circle, divide-square, dollar-sign, download, external-link, feather, frown, hash, headphones, help-circle, info, italic, key, layout, life-buoy, link-2, link, loader, lock, log-in, log-out, maximize, meh, minimize, minimize-2, minus-circle, minus-square, minus, monitor, moon, more-horizontal, more-vertical, move, music, navigation-2, navigation, octagon, pause-circle, percent, plus-circle, plus-square, plus, power, radio, rss, search, server, share, shopping-bag, sidebar, smartphone, smile, square, table-2, tablet, target, terminal, trash-2, trash, triangle, tv, type, upload, x-circle, x-octagon, x-square, x, zoom-in, zoom-out
 *
 * The MIT License (MIT) (for the icons listed above)
 *
 * Copyright (c) 2013-present Cole Bemis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */const a=[["path",{d:"M18 6 6 18"}],["path",{d:"m6 6 12 12"}]];Ne(e,Ae({name:"x"},()=>r,{get iconNode(){return a},children:(o,s)=>{var i=ue(),l=ie(i);$e(l,t,"default",{}),m(o,i)},$$slots:{default:!0}}))}const Nl=(e,t=gr)=>{var r=ue(),a=ie(r);zi(a,t,(o,s)=>{s(o,{size:15,strokeWidth:1.5})}),m(e,r)};var Tl=w("<button><!> </button>"),Pl=w('<div class="fixed inset-0 bg-black/40 z-30 md:hidden"></div>'),Il=w(`<button class="md:hidden fixed top-3 left-3 z-50 p-2 bg-(--color-surface-2) border border-(--color-border)" aria-label="Toggle navigation"><!></button> <aside><div class="px-4 py-3 border-b border-(--color-border)"><h1 class="text-sm font-semibold tracking-wide uppercase text-(--color-accent)">Kagami</h1> <p class="text-xs text-(--color-text-muted) mt-0.5"> </p></div> <nav class="flex-1 overflow-y-auto py-1"><!> <a href="/grafana/" target="_blank" rel="noopener" class="w-full flex items-center gap-2.5 px-4 py-1.5 text-sm text-(--color-text-secondary)
             hover:bg-(--color-surface-2) hover:text-(--color-text-primary)"><!> Monitoring</a></nav> <div class="px-3 py-2 border-t border-(--color-border) flex items-center justify-between"><div class="flex items-center gap-1"><button class="flex items-center gap-1.5 px-2 py-1 text-xs text-(--color-text-muted) hover:text-(--color-text-primary) transition-colors"><!> Logout</button> <button class="flex items-center gap-1 px-2 py-1 text-xs text-(--color-text-muted) hover:text-red-500 transition-colors" title="Shut down server"><!></button></div> <button class="p-1 text-(--color-text-muted) hover:text-(--color-text-primary) transition-colors" aria-label="Toggle theme"><!></button></div></aside> <!>`,1);function Ml(e,t){Ke(t,!0);let r=Yt(t,"currentPage",3,"dashboard"),a=W(!1),o=W(!document.documentElement.classList.contains("light"));function s(){S(o,!n(o)),document.documentElement.classList.toggle("light",!n(o)),localStorage.setItem("kagami_theme",n(o)?"dark":"light")}Je(()=>{localStorage.getItem("kagami_theme")==="light"&&(S(o,!1),document.documentElement.classList.add("light"))});const i=[{page:"dashboard",label:"Dashboard",icon:hl},{page:"sessions",label:"Players",icon:El},{page:"traffic",label:"Traffic",icon:ml},{page:"config",label:"Config",icon:$l},{page:"bans",label:"Bans",icon:ul},{page:"moderation",label:"Moderation",icon:kl},{page:"areas",label:"Areas",icon:xl},{page:"users",label:"Accounts",icon:Xn},{page:"firewall",label:"Firewall",icon:vl},{page:"content",label:"Content",icon:pl}];function l(N){window.location.hash="#/"+N,S(a,!1)}async function c(){await nl(),window.location.hash="#/login"}async function d(){confirm("Shut down the server? All connections will be closed.")&&await sr("/admin/stop")}var f=Il(),g=ie(f),b=u(g);{var y=N=>{Al(N,{size:18})},p=N=>{bl(N,{size:18})};K(b,N=>{n(a)?N(y):N(p,-1)})}var k=_(g,2),x=u(k),v=_(u(x),2),A=u(v),C=_(x,2),E=u(C);Se(E,17,()=>i,ke,(N,Y)=>{let ee=()=>n(Y).page,ce=()=>n(Y).label,ae=()=>n(Y).icon;var $=Tl(),L=u($);Nl(L,ae);var q=_(L);F(()=>{Pe($,1,`w-full flex items-center gap-2.5 px-4 py-1.5 text-sm transition-colors
               ${r()===ee()?"bg-(--color-surface-2) text-(--color-accent) font-medium":"text-(--color-text-secondary) hover:bg-(--color-surface-2) hover:text-(--color-text-primary)"}`),P(q,` ${ce()??""}`)}),Z("click",$,()=>l(ee())),m(N,$)});var O=_(E,2),D=u(O);fl(D,{size:15,strokeWidth:1.5});var T=_(C,2),z=u(T),I=u(z),H=u(I);_l(H,{size:13,strokeWidth:1.5});var B=_(I,2),J=u(B);yl(J,{size:13,strokeWidth:1.5});var R=_(z,2),j=u(R);{var V=N=>{Sl(N,{size:14,strokeWidth:1.5})},G=N=>{gl(N,{size:14,strokeWidth:1.5})};K(j,N=>{n(o)?N(V):N(G,-1)})}var le=_(k,2);{var M=N=>{var Y=Pl();Z("click",Y,()=>S(a,!1)),m(N,Y)};K(le,N=>{n(a)&&N(M)})}F(()=>{Pe(k,1,`fixed md:static inset-y-0 left-0 z-40 w-52 bg-(--color-surface-1) border-r border-(--color-border)
         flex flex-col transition-transform duration-150
         ${n(a)?"translate-x-0":"-translate-x-full"} md:translate-x-0`),P(A,`${se.username??""} · ${se.acl??""}`)}),Z("click",g,()=>S(a,!n(a))),Z("click",I,c),Z("click",B,d),Z("click",R,s),m(e,f),Ge()}Ut(["click"]);var Ol=w('<div class="text-xs text-amber-500 bg-amber-500/10 border border-amber-500/20 px-3 py-2 mb-4"> </div>'),Cl=w('<div class="text-xs text-red-500 bg-red-500/10 border border-red-500/20 px-3 py-2"> </div>'),Ll=w(`<div class="min-h-screen bg-(--color-surface-0) flex items-center justify-center px-4"><div class="w-full max-w-xs"><div class="bg-(--color-surface-1) border border-(--color-border) p-6"><div class="flex items-center gap-2 mb-1"><!> <h1 class="text-lg font-semibold tracking-wide uppercase">Kagami</h1></div> <p class="text-xs text-(--color-text-muted) mb-6">Server Administration</p> <!> <form class="space-y-4"><div><label for="username" class="block text-xs font-medium text-(--color-text-secondary) mb-1">Username</label> <input id="username" type="text" required="" autocomplete="username" class="w-full px-3 py-2 bg-(--color-surface-2) border border-(--color-border) text-sm text-(--color-text-primary)
                   placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active)" placeholder="root"/></div> <div><label for="password" class="block text-xs font-medium text-(--color-text-secondary) mb-1">Password</label> <input id="password" type="password" required="" autocomplete="current-password" class="w-full px-3 py-2 bg-(--color-surface-2) border border-(--color-border) text-sm text-(--color-text-primary)
                   placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active)"/></div> <!> <button type="submit" class="w-full py-2 px-4 bg-(--color-accent) text-(--color-surface-0) text-sm font-medium
                 hover:opacity-80 disabled:opacity-30 transition-opacity"> </button></form></div></div></div>`);function Rl(e,t){Ke(t,!0);let r=W(""),a=W(""),o=W(""),s=W(!1);async function i(T){T.preventDefault(),S(o,""),S(s,!0),se.logoutReason="";const z=await ol(n(r),n(a));S(s,!1),z.ok||S(o,z.error,!0)}var l=Ll(),c=u(l),d=u(c),f=u(d),g=u(f);Xn(g,{size:18,strokeWidth:1.5});var b=_(f,4);{var y=T=>{var z=Ol(),I=u(z);F(()=>P(I,se.logoutReason)),m(T,z)};K(b,T=>{se.logoutReason&&T(y)})}var p=_(b,2),k=u(p),x=_(u(k),2),v=_(k,2),A=_(u(v),2),C=_(v,2);{var E=T=>{var z=Cl(),I=u(z);F(()=>P(I,n(o))),m(T,z)};K(C,T=>{n(o)&&T(E)})}var O=_(C,2),D=u(O);F(()=>{O.disabled=n(s),P(D,n(s)?"Signing in...":"Sign in")}),Un("submit",p,i),nr(x,()=>n(r),T=>S(r,T)),nr(A,()=>n(a),T=>S(a,T)),m(e,l),Ge()}const Tr=(e,t=gr,r=gr,a=gr)=>{var o=Dl(),s=u(o),i=u(s),l=_(i),c=u(l),d=_(s,2),f=u(d);F(()=>{P(i,r()),P(c,a()),P(f,t())}),m(e,o)};var Dl=w('<div class="bg-(--color-surface-1) p-3"><div class="text-xl font-semibold tabular-nums"> <span class="text-xs text-(--color-text-muted) font-normal"> </span></div> <div class="text-[10px] uppercase tracking-wider text-(--color-text-muted) mt-0.5"> </div></div>'),zl=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),jl=w('<span class="text-xs text-(--color-text-secondary)"> </span>'),Fl=w('<div class="bg-(--color-surface-1) border border-(--color-border) px-4 py-3 flex flex-wrap items-baseline gap-x-3 gap-y-1"><span class="font-semibold text-sm"> </span> <span class="text-xs text-(--color-text-muted)"> </span> <!></div>'),Bl=w('<span class="text-[10px] px-1 py-px bg-cyan-500/15 text-cyan-400 font-medium"> </span>'),Wl=w('<span class="text-[10px] px-1 py-px bg-amber-500/15 text-amber-400 font-medium"> </span>'),Vl=w('<div class="px-4 py-1.5 flex items-center justify-between text-sm"><div class="flex items-center gap-2"><span> </span> <!> <!></div> <span class="text-xs text-(--color-text-muted) tabular-nums"> </span></div>'),Hl=w('<a href="#/sessions" class="text-xs text-(--color-text-muted) hover:text-(--color-text-primary)">View all</a>'),Ul=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1"> </td><td class="px-4 py-1 text-(--color-text-secondary)"> </td><td class="px-4 py-1 text-right text-(--color-text-muted) tabular-nums"> </td></tr>'),ql=w('<tr><td colspan="3" class="px-4 py-4 text-center text-(--color-text-muted) text-xs">No active sessions</td></tr>'),Jl=w('<!> <div class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-6 gap-px bg-(--color-border)"><!> <!> <!> <!> <!> <!></div> <div class="grid lg:grid-cols-2 gap-5"><div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="px-4 py-2 border-b border-(--color-border)"><h3 class="text-xs font-semibold uppercase tracking-wide text-(--color-text-muted)">Areas</h3></div> <div class="divide-y divide-(--color-border)"></div></div> <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="px-4 py-2 border-b border-(--color-border) flex items-center justify-between"><h3 class="text-xs font-semibold uppercase tracking-wide text-(--color-text-muted)">Active Sessions</h3> <!></div> <div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-1.5">Name</th><th class="px-4 py-1.5">Area</th><th class="px-4 py-1.5 text-right">Idle</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div></div>',1),Kl=w('<div class="space-y-5"><h2 class="text-lg font-semibold">Dashboard</h2> <!></div>');function Gl(e,t){Ke(t,!0);let r=W(null),a=W(me([])),o=W(me([])),s=W(me(new Map)),i=W(!0);async function l(){var v;const[y,p,k,x]=await Promise.all([rt("/server"),rt("/admin/sessions"),rt("/areas"),al()]);y.status===200&&S(r,y.data,!0),p.status===200&&S(a,p.data,!0),k.status===200&&S(o,((v=k.data)==null?void 0:v.areas)||[],!0),S(s,x,!0),S(i,!1)}Je(()=>{l();const y=setInterval(l,1e4);return()=>clearInterval(y)});function c(y){return n(s).get(y)??0}var d=Kl(),f=_(u(d),2);{var g=y=>{var p=zl();m(y,p)},b=y=>{var p=Jl(),k=ie(p);{var x=M=>{var N=Fl(),Y=u(N),ee=u(Y),ce=_(Y,2),ae=u(ce),$=_(ce,2);{var L=q=>{var Q=jl(),_e=u(Q);F(()=>P(_e,n(r).description)),m(q,Q)};K($,q=>{n(r).description&&q(L)})}F(()=>{P(ee,n(r).name),P(ae,`v${n(r).version??""}`)}),m(M,N)};K(k,M=>{n(r)&&M(x)})}var v=_(k,2),A=u(v);Tr(A,()=>"Players",()=>{var M;return((M=n(r))==null?void 0:M.online)??0},()=>{var M;return"/"+(((M=n(r))==null?void 0:M.max)??"?")});var C=_(A,2);{let M=pe(()=>c("kagami_ws_connections"));Tr(C,()=>"WS Clients",()=>n(M),()=>"")}var E=_(C,2);Tr(E,()=>"REST",()=>n(a).length,()=>"");var O=_(E,2);Tr(O,()=>"Areas",()=>n(o).length,()=>"");var D=_(O,2);{let M=pe(()=>c("kagami_sessions_moderators"));Tr(D,()=>"Mods",()=>n(M),()=>"")}var T=_(D,2);{let M=pe(()=>c("kagami_characters_taken"));Tr(T,()=>"Chars",()=>n(M),()=>"")}var z=_(v,2),I=u(z),H=_(u(I),2);Se(H,21,()=>n(o),ke,(M,N)=>{var Y=Vl(),ee=u(Y),ce=u(ee),ae=u(ce),$=_(ce,2);{var L=xe=>{var Me=Bl(),fe=u(Me);F(()=>P(fe,n(N).status)),m(xe,Me)};K($,xe=>{n(N).status&&n(N).status!=="IDLE"&&xe(L)})}var q=_($,2);{var Q=xe=>{var Me=Wl(),fe=u(Me);F(()=>P(fe,n(N).locked)),m(xe,Me)};K(q,xe=>{n(N).locked&&n(N).locked!=="FREE"&&xe(Q)})}var _e=_(ee,2),Te=u(_e);F(()=>{P(ae,n(N).name),P(Te,n(N).players??0)}),m(M,Y)});var B=_(I,2),J=u(B),R=_(u(J),2);{var j=M=>{var N=Hl();m(M,N)};K(R,M=>{n(a).length>15&&M(j)})}var V=_(J,2),G=u(V),le=_(u(G));Se(le,21,()=>n(a).slice(0,15),ke,(M,N)=>{var Y=Ul(),ee=u(Y),ce=u(ee),ae=_(ee),$=u(ae),L=_(ae),q=u(L);F(()=>{P(ce,n(N).display_name||"(anon)"),P($,n(N).area),P(q,`${n(N).idle_seconds??""}s`)}),m(M,Y)},M=>{var N=ql();m(M,N)}),m(y,p)};K(f,y=>{n(i)?y(g):y(b,-1)})}m(e,d),Ge()}var Yl=w('<span class="text-(--color-text-muted) font-mono truncate max-w-20"> </span>'),Zl=w('<button class="w-full px-3 py-1.5 text-left text-xs hover:bg-(--color-surface-2) flex justify-between gap-2"><span class="text-(--color-text-secondary) truncate"> </span> <!></button>'),Xl=w('<div class="absolute top-full left-0 right-0 z-10 mt-px bg-(--color-surface-1) border border-(--color-border) max-h-60 overflow-y-auto"></div>'),Ql=w('<div class="text-xs text-red-500 bg-red-500/10 border border-red-500/20 px-3 py-2"> </div>'),ec=w("<pre> </pre>"),tc=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),rc=w('<span class="text-[10px] text-(--color-text-muted)">&#x203A;</span>'),ac=w('<button><span class="truncate"> </span> <!></button>'),oc=w('<span class="text-[10px] text-(--color-text-muted)">&#x203A;</span>'),nc=w('<span class="text-[10px] text-(--color-text-muted) font-mono truncate ml-2 max-w-16"> </span>'),sc=w('<button><span class="truncate"> </span> <!></button>'),ic=w('<div class="w-48 shrink-0 border-r border-(--color-border) overflow-y-auto bg-(--color-surface-1)"></div>'),lc=w('<p class="text-(--color-text-muted) text-sm">Select a key to edit</p>'),cc=w('<input type="checkbox" class="accent-(--color-accent)"/>'),dc=w('<input type="number" step="any" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"/>'),uc=w('<textarea rows="2" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"></textarea>'),fc=w('<input type="text" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)"/>'),vc=w('<div class="flex items-center gap-2"><span class="text-[10px] text-(--color-text-muted) w-5 text-right tabular-nums"></span> <!> <button class="p-1 text-(--color-text-muted) hover:text-red-400"><!></button></div>'),pc=w('<div class="space-y-1"><!> <button class="flex items-center gap-1 mt-2 px-2 py-1 text-xs text-(--color-text-muted) hover:text-(--color-text-primary) border border-dashed border-(--color-border) hover:border-(--color-border-active)"><!> Add item</button></div>'),hc=w('<input type="checkbox" class="accent-(--color-accent)"/>'),_c=w('<input type="number" step="any" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"/>'),xc=w('<input type="text" class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)"/>'),bc=w('<div class="flex items-center gap-3"><span class="text-xs text-(--color-text-secondary) w-44 shrink-0 truncate"> </span> <!></div>'),mc=w('<div class="space-y-2"></div>'),gc=w('<input type="checkbox" class="accent-(--color-accent)"/> <span class="text-sm"> </span>',1),yc=w('<input type="number" step="any" class="w-48 px-2 py-1 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"/>'),wc=w('<input type="text" class="flex-1 px-2 py-1 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)"/>'),$c=w('<div class="flex items-center gap-3"><!></div>'),kc=w('<h3 class="text-xs font-semibold uppercase tracking-wider text-(--color-text-muted) mb-3"> </h3> <!>',1),Sc=w('<div class="flex border border-(--color-border) overflow-hidden" style="height: 65vh"><div class="w-48 shrink-0 border-r border-(--color-border) overflow-y-auto bg-(--color-surface-1)"></div> <!> <div class="flex-1 overflow-y-auto bg-(--color-surface-0) p-4"><!></div></div>'),Ec=w(`<div class="space-y-3"><div class="flex items-center justify-between flex-wrap gap-2"><h2 class="text-lg font-semibold">Configuration</h2> <div class="flex gap-px items-center"><div class="relative"><!> <input type="text" placeholder="Search keys..." class="pl-8 pr-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
                 placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active) w-48"/> <!></div> <button class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-(--color-accent) text-(--color-surface-0) hover:opacity-80 disabled:opacity-30"><!> </button></div></div> <!> <!> <!></div>`);function Ac(e,t){Ke(t,!0);let r=W(null),a=W(null),o=W(!0),s=W(!1),i=W(""),l=W(""),c=W(""),d=W(""),f=W(me([]));async function g(){var L;S(o,!0);const $=await rt("/admin/config");$.status===200?(S(r,$.data,!0),S(a,JSON.parse(JSON.stringify($.data)),!0),S(f,[],!0)):S(c,((L=$.data)==null?void 0:L.reason)||"Failed to load config",!0),S(o,!1)}Je(()=>{g()});function b($){let L=n(r);for(let q=0;q<$;q++){if(!L||typeof L!="object")return null;L=L[n(f)[q]]}return L}function y($,L){S(f,[...n(f).slice(0,$),L],!0)}function p($){return $!==null&&typeof $=="object"&&!Array.isArray($)}function k($,L){if($===L)return{};if(typeof L!="object"||L===null||Array.isArray(L))return JSON.stringify(L)!==JSON.stringify($)?L:{};const q={};for(const Q of Object.keys(L))if(p(L[Q])&&p($==null?void 0:$[Q])){const _e=k($[Q],L[Q]);Object.keys(_e).length>0&&(q[Q]=_e)}else JSON.stringify(L[Q])!==JSON.stringify($==null?void 0:$[Q])&&(q[Q]=L[Q]);return q}async function x(){var q,Q;S(c,""),S(i,"");const $=k(n(a),n(r));if(typeof $=="object"&&Object.keys($).length===0){S(i,"No changes."),S(l,"info");return}S(s,!0);const L=await rl("/admin/config",typeof $=="object"?$:n(r));S(s,!1),L.status===200?(S(i,((q=L.data)==null?void 0:q.reload_summary)||"Saved and applied.",!0),S(l,"ok"),S(a,JSON.parse(JSON.stringify(n(r))),!0)):S(c,((Q=L.data)==null?void 0:Q.reason)||"Failed",!0)}function v($,L=""){const q=[];if(!$||typeof $!="object")return q;for(const[Q,_e]of Object.entries($)){const Te=L?`${L}/${Q}`:Q;q.push({path:Te,key:Q,value:_e}),p(_e)&&q.push(...v(_e,Te))}return q}let A=pe(()=>{if(!n(d)||!n(r))return[];const $=n(d).toLowerCase();return v(n(r)).filter(L=>L.path.toLowerCase().includes($)||typeof L.value!="object"&&String(L.value).toLowerCase().includes($)).slice(0,30)});function C($){const L=$.split("/");S(f,[],!0);let q=n(r);for(const Q of L)if(q&&p(q)&&Q in q)S(f,[...n(f),Q],!0),q=q[Q];else break;S(d,"")}function E($){if($.length>0){const L=$[$.length-1];typeof L=="string"?$.push(""):typeof L=="number"?$.push(0):typeof L=="boolean"?$.push(!1):$.push("")}else $.push("")}function O($,L){$.splice(L,1)}var D=Ec(),T=u(D),z=_(u(T),2),I=u(z),H=u(I);wl(H,{size:13,class:"absolute left-2.5 top-1/2 -translate-y-1/2 text-(--color-text-muted)",strokeWidth:1.5});var B=_(H,2),J=_(B,2);{var R=$=>{var L=Xl();Se(L,21,()=>n(A),ke,(q,Q)=>{var _e=Zl(),Te=u(_e),xe=u(Te),Me=_(Te,2);{var fe=be=>{var oe=Yl(),He=u(oe);F(Ye=>P(He,Ye),[()=>String(n(Q).value)]),m(be,oe)},ve=pe(()=>!p(n(Q).value));K(Me,be=>{n(ve)&&be(fe)})}F(()=>P(xe,n(Q).path)),Z("click",_e,()=>C(n(Q).path)),m(q,_e)}),m($,L)};K(J,$=>{n(A).length>0&&$(R)})}var j=_(I,2),V=u(j);es(V,{size:13,strokeWidth:1.5});var G=_(V),le=_(T,2);{var M=$=>{var L=Ql(),q=u(L);F(()=>P(q,n(c))),m($,L)};K(le,$=>{n(c)&&$(M)})}var N=_(le,2);{var Y=$=>{var L=ec(),q=u(L);F(()=>{Pe(L,1,`text-xs ${n(l)==="ok"?"text-emerald-400 bg-emerald-400/10 border-emerald-400/20":"text-(--color-text-secondary) bg-(--color-surface-2) border-(--color-border)"} border px-3 py-2 whitespace-pre-wrap`),P(q,n(i))}),m($,L)};K(N,$=>{n(i)&&$(Y)})}var ee=_(N,2);{var ce=$=>{var L=tc();m($,L)},ae=$=>{var L=Sc(),q=u(L);Se(q,21,()=>Object.keys(n(r)),ke,(fe,ve)=>{const be=pe(()=>n(r)[n(ve)]);var oe=ac(),He=u(oe),Ye=u(He),Ot=_(He,2);{var ir=Ze=>{var Ar=rc();m(Ze,Ar)},lr=pe(()=>p(n(be))||Array.isArray(n(be)));K(Ot,Ze=>{n(lr)&&Ze(ir)})}F(()=>{Pe(oe,1,`w-full px-3 py-1.5 text-left text-sm flex items-center justify-between hover:bg-(--color-surface-2)
                   ${n(f)[0]===n(ve)?"bg-(--color-surface-2) text-(--color-accent) font-medium":"text-(--color-text-secondary)"}`),P(Ye,n(ve))}),Z("click",oe,()=>y(0,n(ve))),m(fe,oe)});var Q=_(q,2);Se(Q,17,()=>n(f),ke,(fe,ve,be)=>{const oe=pe(()=>b(be)),He=pe(()=>{var Ze;return(Ze=n(oe))==null?void 0:Ze[n(ve)]});var Ye=ue(),Ot=ie(Ye);{var ir=Ze=>{var Ar=ic();Se(Ar,21,()=>Object.keys(n(He)),ke,(za,Nr)=>{const De=pe(()=>n(He)[n(Nr)]);var at=sc(),Ct=u(at),qt=u(Ct),Jt=_(Ct,2);{var bt=Oe=>{var mt=oc();m(Oe,mt)},de=pe(()=>p(n(De))||Array.isArray(n(De))),ye=Oe=>{var mt=nc(),Kt=u(mt);F(Lt=>P(Kt,Lt),[()=>JSON.stringify(n(De))]),m(Oe,mt)};K(Jt,Oe=>{n(de)?Oe(bt):Oe(ye,-1)})}F(()=>{Pe(at,1,`w-full px-3 py-1.5 text-left text-sm flex items-center justify-between hover:bg-(--color-surface-2)
                       ${n(f)[be+1]===n(Nr)?"bg-(--color-surface-2) text-(--color-accent) font-medium":"text-(--color-text-secondary)"}`),P(qt,n(Nr))}),Z("click",at,()=>y(be+1,n(Nr))),m(za,at)}),m(Ze,Ar)},lr=pe(()=>p(n(He)));K(Ot,Ze=>{n(lr)&&Ze(ir)})}m(fe,Ye)});var _e=_(Q,2),Te=u(_e);{var xe=fe=>{var ve=lc();m(fe,ve)},Me=fe=>{const ve=pe(()=>b(n(f).length-1)),be=pe(()=>n(f)[n(f).length-1]),oe=pe(()=>{var De;return(De=n(ve))==null?void 0:De[n(be)]});var He=kc(),Ye=ie(He),Ot=u(Ye),ir=_(Ye,2);{var lr=De=>{var at=pc(),Ct=u(at);Se(Ct,17,()=>n(oe),ke,(bt,de,ye)=>{var Oe=vc(),mt=u(Oe);mt.textContent=ye;var Kt=_(mt,2);{var Lt=Xe=>{var je=cc();F(()=>qa(je,n(de))),Z("change",je,Ce=>n(oe)[ye]=Ce.target.checked),m(Xe,je)},xa=Xe=>{var je=dc();F(()=>dr(je,n(de))),Z("change",je,Ce=>n(oe)[ye]=Number(Ce.target.value)),m(Xe,je)},Hr=Xe=>{var je=uc();F(Ce=>dr(je,Ce),[()=>JSON.stringify(n(de),null,2)]),Z("change",je,Ce=>{try{n(oe)[ye]=JSON.parse(Ce.target.value)}catch{}}),m(Xe,je)},ja=Xe=>{var je=fc();F(()=>dr(je,n(de))),Z("input",je,Ce=>n(oe)[ye]=Ce.target.value),m(Xe,je)};K(Kt,Xe=>{typeof n(de)=="boolean"?Xe(Lt):typeof n(de)=="number"?Xe(xa,1):typeof n(de)=="object"?Xe(Hr,2):Xe(ja,-1)})}var ba=_(Kt,2),Fa=u(ba);uo(Fa,{size:12,strokeWidth:1.5}),Z("click",ba,()=>O(n(oe),ye)),m(bt,Oe)});var qt=_(Ct,2),Jt=u(qt);Qn(Jt,{size:12,strokeWidth:1.5}),Z("click",qt,()=>E(n(oe))),m(De,at)},Ze=pe(()=>Array.isArray(n(oe))),Ar=De=>{var at=mc();Se(at,21,()=>Object.entries(n(oe)),ke,(Ct,qt)=>{var Jt=pe(()=>ho(n(qt),2));let bt=()=>n(Jt)[0],de=()=>n(Jt)[1];var ye=ue(),Oe=ie(ye);{var mt=Lt=>{var xa=bc(),Hr=u(xa),ja=u(Hr),ba=_(Hr,2);{var Fa=Ce=>{var gt=hc();F(()=>qa(gt,de())),Z("change",gt,Ur=>n(oe)[bt()]=Ur.target.checked),m(Ce,gt)},Xe=Ce=>{var gt=_c();F(()=>dr(gt,de())),Z("change",gt,Ur=>n(oe)[bt()]=Number(Ur.target.value)),m(Ce,gt)},je=Ce=>{var gt=xc();F(()=>dr(gt,de()??"")),Z("input",gt,Ur=>n(oe)[bt()]=Ur.target.value),m(Ce,gt)};K(ba,Ce=>{typeof de()=="boolean"?Ce(Fa):typeof de()=="number"?Ce(Xe,1):Ce(je,-1)})}F(()=>{Ta(Hr,"title",bt()),P(ja,bt())}),m(Lt,xa)},Kt=pe(()=>!p(de())&&!Array.isArray(de()));K(Oe,Lt=>{n(Kt)&&Lt(mt)})}m(Ct,ye)}),m(De,at)},za=pe(()=>p(n(oe))),Nr=De=>{var at=$c(),Ct=u(at);{var qt=de=>{var ye=gc(),Oe=ie(ye),mt=_(Oe,2),Kt=u(mt);F(()=>{qa(Oe,n(oe)),P(Kt,n(oe)?"true":"false")}),Z("change",Oe,Lt=>n(ve)[n(be)]=Lt.target.checked),m(de,ye)},Jt=de=>{var ye=yc();F(()=>dr(ye,n(oe))),Z("change",ye,Oe=>n(ve)[n(be)]=Number(Oe.target.value)),m(de,ye)},bt=de=>{var ye=wc();F(()=>dr(ye,n(oe)??"")),Z("input",ye,Oe=>n(ve)[n(be)]=Oe.target.value),m(de,ye)};K(Ct,de=>{typeof n(oe)=="boolean"?de(qt):typeof n(oe)=="number"?de(Jt,1):de(bt,-1)})}m(De,at)};K(ir,De=>{n(Ze)?De(lr):n(za)?De(Ar,1):De(Nr,-1)})}F(De=>P(Ot,De),[()=>n(f).join(" / ")]),m(fe,He)};K(Te,fe=>{n(f).length===0?fe(xe):fe(Me,-1)})}m($,L)};K(ee,$=>{n(o)?$(ce):n(r)&&$(ae,1)})}F(()=>{j.disabled=n(s),P(G,` ${n(s)?"Saving...":"Save & Apply"}`)}),nr(B,()=>n(d),$=>S(d,$)),Z("click",j,x),m(e,D),Ge()}Ut(["click","change","input"]);const ct=(e,t=gr,r=gr)=>{var a=Nc(),o=u(a),s=u(o),i=_(o,2),l=u(i);F(()=>{P(s,t()),P(l,r())}),m(e,a)};var Nc=w('<div><div class="text-(--color-text-muted) text-[10px] uppercase tracking-wider"> </div> <div class="text-(--color-text-primary) font-mono mt-0.5 truncate"> </div></div>'),Tc=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Pc=w('<tr><td class="px-4 py-1.5 font-medium"> </td><td class="px-4 py-1.5 text-(--color-text-secondary)"> </td><td class="px-4 py-1.5 text-(--color-text-secondary)"> </td><td class="px-4 py-1.5 text-(--color-text-muted) hidden sm:table-cell truncate max-w-32"> </td><td class="px-4 py-1.5 text-right text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td><td class="px-4 py-1.5 text-right text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td><td class="px-4 py-1.5 text-right text-(--color-text-muted) tabular-nums"> </td></tr>'),Ic=w('<div class="bg-(--color-surface-1) border border-(--color-border) p-4 space-y-3"><h3 class="text-sm font-semibold"> </h3> <div class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-4 gap-3 text-xs"><!> <!> <!> <!> <!> <!> <!> <!> <!> <!> <!> <!></div></div>'),Mc=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">Name</th><th class="px-4 py-2">Protocol</th><th class="px-4 py-2">Area</th><th class="px-4 py-2 hidden sm:table-cell">Client</th><th class="px-4 py-2 text-right hidden md:table-cell">Sent</th><th class="px-4 py-2 text-right hidden md:table-cell">Recv</th><th class="px-4 py-2 text-right">Idle</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div> <!>',1),Oc=w('<div class="space-y-4"><h2 class="text-lg font-semibold">Players <span class="text-sm text-(--color-text-muted) font-normal"> </span></h2> <!></div>');function Cc(e,t){Ke(t,!0);let r=W(me([])),a=W(!0),o=W(null);async function s(){const p=await rt("/admin/sessions");p.status===200&&S(r,p.data,!0),S(a,!1)}Je(()=>{s();const p=setInterval(s,5e3);return()=>clearInterval(p)});function i(p){return p<1024?p+" B":p<1024*1024?(p/1024).toFixed(1)+" KB":(p/(1024*1024)).toFixed(1)+" MB"}var l=Oc(),c=u(l),d=_(u(c)),f=u(d),g=_(c,2);{var b=p=>{var k=Tc();m(p,k)},y=p=>{var k=Mc(),x=ie(k),v=u(x),A=u(v),C=_(u(A));Se(C,21,()=>n(r),ke,(D,T)=>{var z=Pc(),I=u(z),H=u(I),B=_(I),J=u(B),R=_(B),j=u(R),V=_(R),G=u(V),le=_(V),M=u(le),N=_(le),Y=u(N),ee=_(N),ce=u(ee);F((ae,$)=>{var L;Pe(z,1,`hover:bg-(--color-surface-2)/50 cursor-pointer ${((L=n(o))==null?void 0:L.session_id)===n(T).session_id?"bg-(--color-surface-2)":""}`),P(H,n(T).display_name||"(anon)"),P(J,n(T).protocol),P(j,n(T).area),P(G,n(T).client_software),P(M,ae),P(Y,$),P(ce,`${n(T).idle_seconds??""}s`)},[()=>i(n(T).bytes_sent),()=>i(n(T).bytes_received)]),Z("click",z,()=>{var ae;return S(o,((ae=n(o))==null?void 0:ae.session_id)===n(T).session_id?null:n(T),!0)}),m(D,z)});var E=_(x,2);{var O=D=>{var T=Ic(),z=u(T),I=u(z),H=_(z,2),B=u(H);ct(B,()=>"Session ID",()=>n(o).session_id);var J=_(B,2);ct(J,()=>"Protocol",()=>n(o).protocol);var R=_(J,2);ct(R,()=>"Area",()=>n(o).area);var j=_(R,2);ct(j,()=>"Character",()=>n(o).character_id>=0?"#"+n(o).character_id:"None");var V=_(j,2);ct(V,()=>"HDID",()=>n(o).hardware_id);var G=_(V,2);ct(G,()=>"Client",()=>n(o).client_software);var le=_(G,2);ct(le,()=>"Packets Sent",()=>n(o).packets_sent);var M=_(le,2);ct(M,()=>"Packets Recv",()=>n(o).packets_received);var N=_(M,2);ct(N,()=>"Mod Actions",()=>n(o).mod_actions);var Y=_(N,2);{let ae=pe(()=>i(n(o).bytes_sent));ct(Y,()=>"Bytes Sent",()=>n(ae))}var ee=_(Y,2);{let ae=pe(()=>i(n(o).bytes_received));ct(ee,()=>"Bytes Recv",()=>n(ae))}var ce=_(ee,2);ct(ce,()=>"Idle",()=>n(o).idle_seconds+"s"),F(()=>P(I,n(o).display_name||"(anonymous)")),m(D,T)};K(E,D=>{n(o)&&D(O)})}m(p,k)};K(g,p=>{n(a)?p(b):p(y,-1)})}F(()=>P(f,`(${n(r).length??""})`)),m(e,l),Ge()}Ut(["click"]);var Lc=w('<span class="text-(--color-text-muted) shrink-0"> </span>'),Rc=w('<div class="px-3 py-1 flex gap-2 hover:bg-(--color-surface-2)/50 border-b border-(--color-border)/30"><span class="text-(--color-text-muted) shrink-0 w-16"> </span> <span> </span> <!> <span class="text-amber-400 shrink-0"> </span> <span class="text-(--color-text-primary) break-all"> </span></div>'),Dc=w('<div class="px-4 py-12 text-center text-(--color-text-muted)"> </div>'),zc=w(`<div class="space-y-4"><div class="flex items-center justify-between flex-wrap gap-2"><div class="flex items-center gap-2"><h2 class="text-lg font-semibold">Traffic</h2> <span></span></div> <input type="text" placeholder="Filter..." class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
             placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active) w-56"/></div> <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="max-h-[75vh] overflow-y-auto font-mono text-xs leading-relaxed"></div></div></div>`);function jc(e,t){Ke(t,!0);let r=W(me([])),a=W("disconnected"),o=W(""),s=500;function i(){const v=Zn();v&&l(v)}async function l(v){S(a,"connecting");try{const A=await fetch("/aonx/v1/events",{headers:{Authorization:`Bearer ${v}`}});if(!A.ok){S(a,"disconnected");return}S(a,"connected");const C=A.body.getReader(),E=new TextDecoder;let O="";for(;;){const{done:D,value:T}=await C.read();if(D)break;O+=E.decode(T,{stream:!0});const z=O.split(`
`);O=z.pop()||"";let I="",H="";for(const B of z)B.startsWith("event: ")?I=B.slice(7):B.startsWith("data: ")?H=B.slice(6):B===""&&I&&H&&(c(I,H),I="",H="")}}catch(A){console.error("SSE error:",A)}S(a,"disconnected")}function c(v,A){if(!(v!=="ic_message"&&v!=="ooc_message"))try{const C=JSON.parse(A);S(r,[{type:v==="ic_message"?"IC":"OOC",name:C.showname||C.name||C.character||"???",text:C.message||"",time:new Date().toLocaleTimeString("en-US",{hour12:!1}),area:C.area||""},...n(r).slice(0,s-1)],!0)}catch{}}Je(()=>{i()});let d=pe(()=>n(o)?n(r).filter(v=>v.text.toLowerCase().includes(n(o).toLowerCase())||v.name.toLowerCase().includes(n(o).toLowerCase())||v.area.toLowerCase().includes(n(o).toLowerCase())):n(r));var f=zc(),g=u(f),b=u(g),y=_(u(b),2),p=_(b,2),k=_(g,2),x=u(k);Se(x,21,()=>n(d),ke,(v,A)=>{var C=Rc(),E=u(C),O=u(E),D=_(E,2),T=u(D),z=_(D,2);{var I=j=>{var V=Lc(),G=u(V);F(()=>P(G,`[${n(A).area??""}]`)),m(j,V)};K(z,j=>{n(A).area&&j(I)})}var H=_(z,2),B=u(H),J=_(H,2),R=u(J);F(()=>{P(O,n(A).time),Pe(D,1,`shrink-0 w-6 text-center font-bold ${n(A).type==="IC"?"text-cyan-400":"text-emerald-400"}`),P(T,n(A).type),P(B,n(A).name),P(R,n(A).text)}),m(v,C)},v=>{var A=Dc(),C=u(A);F(()=>P(C,n(a)==="connected"?"Waiting for messages...":n(a)==="connecting"?"Connecting...":"Not connected")),m(v,A)}),F(()=>Pe(y,1,`w-1.5 h-1.5 ${n(a)==="connected"?"bg-emerald-500":n(a)==="connecting"?"bg-amber-500 animate-pulse":"bg-red-500"}`)),nr(p,()=>n(o),v=>S(o,v)),m(e,f),Ge()}var Fc=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Bc=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-mono text-xs"> </td><td class="px-4 py-1.5 max-w-xs truncate"> </td><td class="px-4 py-1.5 text-(--color-text-secondary) hidden md:table-cell"> </td><td class="px-4 py-1.5 text-(--color-text-muted) text-xs hidden lg:table-cell"> </td><td class="px-4 py-1.5"><span> </span></td><td class="px-4 py-1.5"><button class="text-xs text-red-400 hover:text-red-300">Unban</button></td></tr>'),Wc=w('<tr><td colspan="6" class="px-4 py-8 text-center text-(--color-text-muted) text-sm">No bans found</td></tr>'),Vc=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">IPID</th><th class="px-4 py-2">Reason</th><th class="px-4 py-2 hidden md:table-cell">By</th><th class="px-4 py-2 hidden lg:table-cell">When</th><th class="px-4 py-2">Duration</th><th class="px-4 py-2"></th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),Hc=w(`<div class="space-y-4"><div class="flex items-center justify-between flex-wrap gap-2"><h2 class="text-lg font-semibold">Bans</h2> <form class="flex gap-px"><input type="text" placeholder="Search IPID, HDID, reason..." class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
               placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active) w-56"/> <button type="submit" class="px-3 py-1.5 text-sm bg-(--color-surface-3) border border-(--color-border) text-(--color-text-secondary) hover:text-(--color-text-primary)">Search</button></form></div> <!></div>`);function Uc(e,t){Ke(t,!0);let r=W(me([])),a=W(""),o=W(!0);async function s(){var A;const x=n(a)?`?query=${encodeURIComponent(n(a))}&limit=100`:"?limit=100",v=await rt("/admin/bans"+x);v.status===200&&S(r,((A=v.data)==null?void 0:A.bans)||[],!0),S(o,!1)}Je(()=>{s()});async function i(x){confirm(`Unban ${x}?`)&&(await sr("/moderation/actions",{action:"unban",target:x}),s())}function l(x){if(x===-2)return"Permanent";if(x===0)return"Invalidated";const v=Math.floor(x/3600),A=Math.floor(x%3600/60);return v>0?`${v}h ${A}m`:`${A}m`}function c(x){return x?new Date(x*1e3).toLocaleString():""}var d=Hc(),f=u(d),g=_(u(f),2),b=u(g),y=_(f,2);{var p=x=>{var v=Fc();m(x,v)},k=x=>{var v=Vc(),A=u(v),C=u(A),E=_(u(C));Se(E,21,()=>n(r),ke,(O,D)=>{var T=Bc(),z=u(T),I=u(z),H=_(z),B=u(H),J=_(H),R=u(J),j=_(J),V=u(j),G=_(j),le=u(G),M=u(le),N=_(G),Y=u(N);F((ee,ce)=>{P(I,n(D).ipid),P(B,n(D).reason),P(R,n(D).moderator),P(V,ee),Pe(le,1,`text-[10px] px-1 py-px font-medium ${n(D).permanent?"bg-red-500/15 text-red-400":"bg-(--color-surface-3) text-(--color-text-muted)"}`),P(M,ce)},[()=>c(n(D).timestamp),()=>l(n(D).duration)]),Z("click",Y,()=>i(n(D).ipid)),m(O,T)},O=>{var D=Wc();m(O,D)}),m(x,v)};K(y,x=>{n(o)?x(p):x(k,-1)})}Un("submit",g,x=>{x.preventDefault(),s()}),nr(b,()=>n(a),x=>S(a,x)),m(e,d),Ge()}Ut(["click"]);var qc=w('<p class="text-xs mt-2 text-(--color-text-muted)"> </p>'),Jc=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Kc=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-3 py-1.5 text-(--color-text-muted)"> </td><td class="px-3 py-1.5 font-mono"> </td><td class="px-3 py-1.5 text-(--color-text-secondary)"> </td><td class="px-3 py-1.5"><span> </span></td><td class="px-3 py-1.5 text-(--color-text-secondary) max-w-xs truncate hidden md:table-cell"> </td><td class="px-3 py-1.5 text-(--color-text-muted) max-w-xs truncate hidden lg:table-cell"> </td></tr>'),Gc=w('<tr><td colspan="6" class="px-4 py-8 text-center text-(--color-text-muted)">No events</td></tr>'),Yc=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-xs"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-3 py-2">Time</th><th class="px-3 py-2">IPID</th><th class="px-3 py-2">Ch</th><th class="px-3 py-2">Action</th><th class="px-3 py-2 hidden md:table-cell">Message</th><th class="px-3 py-2 hidden lg:table-cell">Reason</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),Zc=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-mono text-xs"> </td><td class="px-4 py-1.5 text-(--color-text-secondary)"> </td><td class="px-4 py-1.5"> </td><td class="px-4 py-1.5"><button class="text-xs text-red-400 hover:text-red-300">Unmute</button></td></tr>'),Xc=w('<tr><td colspan="4" class="px-4 py-8 text-center text-(--color-text-muted)">No active mutes</td></tr>'),Qc=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">IPID</th><th class="px-4 py-2">Reason</th><th class="px-4 py-2">Remaining</th><th class="px-4 py-2"></th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),ed=w('<div class="space-y-4"><h2 class="text-lg font-semibold">Moderation</h2> <div class="bg-(--color-surface-1) border border-(--color-border) p-4"><h3 class="text-[10px] font-semibold uppercase tracking-wider text-(--color-text-muted) mb-3">Quick Action</h3> <div class="flex flex-wrap gap-px"><input placeholder="IPID / session ID / IP" class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) placeholder:text-(--color-text-muted) w-44 focus:outline-none focus:border-(--color-border-active)"/> <input placeholder="Reason" class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) placeholder:text-(--color-text-muted) flex-1 min-w-24 focus:outline-none focus:border-(--color-border-active)"/> <button class="px-3 py-1.5 text-sm bg-amber-600 text-white hover:bg-amber-500">Kick</button> <button class="px-3 py-1.5 text-sm bg-orange-600 text-white hover:bg-orange-500">Mute</button> <button class="px-3 py-1.5 text-sm bg-red-600 text-white hover:bg-red-500">Ban</button></div> <!></div> <div class="flex gap-4 border-b border-(--color-border)"><button>Audit Log</button> <button>Active Mutes</button></div> <!></div>');function td(e,t){Ke(t,!0);let r=W("audit"),a=W(me([])),o=W(me([])),s=W(!0),i=W(""),l=W(""),c=W("");const d={0:"none",1:"log",2:"censor",3:"drop",4:"mute",5:"kick",6:"ban",7:"perma_ban",none:"none",log:"log",censor:"censor",drop:"drop",mute:"mute",kick:"kick",ban:"ban",perma_ban:"perma_ban"},f={ban:"bg-red-500/15 text-red-400",perma_ban:"bg-red-500/15 text-red-400",kick:"bg-amber-500/15 text-amber-400",mute:"bg-orange-500/15 text-orange-400",censor:"bg-violet-500/15 text-violet-400",drop:"bg-violet-500/15 text-violet-400",log:"bg-(--color-surface-3) text-(--color-text-muted)",none:"bg-(--color-surface-3) text-(--color-text-muted)"};function g(M){return d[String(M)]||String(M)}function b(M){return f[g(M)]||"bg-(--color-surface-3) text-(--color-text-muted)"}async function y(){var N;S(s,!0);const M=await rt("/admin/moderation-events?limit=200");M.status===200&&S(a,((N=M.data)==null?void 0:N.events)||[],!0),S(s,!1)}async function p(){var N;S(s,!0);const M=await rt("/admin/mutes");M.status===200&&S(o,((N=M.data)==null?void 0:N.mutes)||[],!0),S(s,!1)}Je(()=>{n(r)==="audit"?y():p()});async function k(M){var ee;if(!n(i))return;S(c,"");const N={action:M,target:n(i),reason:n(l)||void 0};M==="mute"&&(N.duration=900);const Y=await sr("/moderation/actions",N);S(c,Y.status===200?`${M} applied`:((ee=Y.data)==null?void 0:ee.reason)||"Failed",!0),S(i,""),S(l,""),n(r)==="mutes"&&p()}async function x(M){await sr("/moderation/actions",{action:"unmute",target:M}),p()}var v=ed(),A=_(u(v),2),C=_(u(A),2),E=u(C),O=_(E,2),D=_(O,2),T=_(D,2),z=_(T,2),I=_(C,2);{var H=M=>{var N=qc(),Y=u(N);F(()=>P(Y,n(c))),m(M,N)};K(I,M=>{n(c)&&M(H)})}var B=_(A,2),J=u(B),R=_(J,2),j=_(B,2);{var V=M=>{var N=Jc();m(M,N)},G=M=>{var N=Yc(),Y=u(N),ee=u(Y),ce=_(u(ee));Se(ce,21,()=>n(a),ke,(ae,$)=>{var L=Kc(),q=u(L),Q=u(q),_e=_(q),Te=u(_e),xe=_(_e),Me=u(xe),fe=_(xe),ve=u(fe),be=u(ve),oe=_(fe),He=u(oe),Ye=_(oe),Ot=u(Ye);F((ir,lr,Ze)=>{P(Q,ir),P(Te,n($).ipid),P(Me,n($).channel),Pe(ve,1,`text-[10px] px-1 py-px font-medium ${lr??""}`),P(be,Ze),P(He,n($).message_sample),P(Ot,n($).reason)},[()=>new Date(n($).timestamp_ms).toLocaleString(),()=>b(n($).action),()=>g(n($).action)]),m(ae,L)},ae=>{var $=Gc();m(ae,$)}),m(M,N)},le=M=>{var N=Qc(),Y=u(N),ee=u(Y),ce=_(u(ee));Se(ce,21,()=>n(o),ke,(ae,$)=>{var L=Zc(),q=u(L),Q=u(q),_e=_(q),Te=u(_e),xe=_(_e),Me=u(xe),fe=_(xe),ve=u(fe);F(be=>{P(Q,n($).ipid),P(Te,n($).reason),P(Me,be)},[()=>n($).seconds_remaining<0?"Permanent":Math.ceil(n($).seconds_remaining/60)+"m"]),Z("click",ve,()=>x(n($).ipid)),m(ae,L)},ae=>{var $=Xc();m(ae,$)}),m(M,N)};K(j,M=>{n(s)?M(V):n(r)==="audit"?M(G,1):M(le,-1)})}F(()=>{Pe(J,1,`pb-2 text-sm border-b-2 transition-colors ${n(r)==="audit"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`),Pe(R,1,`pb-2 text-sm border-b-2 transition-colors ${n(r)==="mutes"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`)}),nr(E,()=>n(i),M=>S(i,M)),nr(O,()=>n(l),M=>S(l,M)),Z("click",D,()=>k("kick")),Z("click",T,()=>k("mute")),Z("click",z,()=>k("ban")),Z("click",J,()=>S(r,"audit")),Z("click",R,()=>S(r,"mutes")),m(e,v),Ge()}Ut(["click"]);var rd=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),ad=w('<span class="px-1 py-px bg-cyan-500/15 text-cyan-400 font-medium"> </span>'),od=w('<span class="px-1 py-px bg-(--color-surface-3) text-(--color-text-muted)">IDLE</span>'),nd=w('<span class="px-1 py-px bg-amber-500/15 text-amber-400 font-medium"> </span>'),sd=w('<button><div class="flex items-center justify-between mb-1"><span class="font-medium text-sm"> </span> <span class="text-xs text-(--color-text-muted) tabular-nums"> </span></div> <div class="flex gap-1 text-[10px]"><!> <!></div></button>'),id=w('<div><span class="text-(--color-text-muted) text-[10px] uppercase">Background</span><div class="mt-0.5"> </div></div>'),ld=w('<div><span class="text-(--color-text-muted) text-[10px] uppercase">Music</span><div class="mt-0.5"> </div></div>'),cd=w('<div><span class="text-(--color-text-muted) text-[10px] uppercase">HP</span><div class="mt-0.5"> </div></div>'),dd=w('<div class="bg-(--color-surface-1) border border-(--color-border) p-4 space-y-3"><h3 class="text-sm font-semibold"> </h3> <div class="grid grid-cols-2 sm:grid-cols-3 gap-3 text-xs"><!> <!> <!></div></div>'),ud=w('<div class="grid gap-px bg-(--color-border) sm:grid-cols-2 lg:grid-cols-3"></div> <!>',1),fd=w('<div class="space-y-4"><h2 class="text-lg font-semibold">Areas</h2> <!></div>');function vd(e,t){Ke(t,!0);let r=W(me([])),a=W(!0),o=W(null);async function s(){var b;const g=await rt("/areas");g.status===200&&S(r,((b=g.data)==null?void 0:b.areas)||[],!0),S(a,!1)}Je(()=>{s();const g=setInterval(s,1e4);return()=>clearInterval(g)});async function i(g){const b=await rt(`/areas/${g}`);b.status===200&&S(o,b.data,!0)}var l=fd(),c=_(u(l),2);{var d=g=>{var b=rd();m(g,b)},f=g=>{var b=ud(),y=ie(b);Se(y,21,()=>n(r),ke,(x,v)=>{var A=sd(),C=u(A),E=u(C),O=u(E),D=_(E,2),T=u(D),z=_(C,2),I=u(z);{var H=j=>{var V=ad(),G=u(V);F(()=>P(G,n(v).status)),m(j,V)},B=j=>{var V=od();m(j,V)};K(I,j=>{n(v).status&&n(v).status!=="IDLE"?j(H):j(B,-1)})}var J=_(I,2);{var R=j=>{var V=nd(),G=u(V);F(()=>P(G,n(v).locked)),m(j,V)};K(J,j=>{n(v).locked&&n(v).locked!=="FREE"&&j(R)})}F(()=>{var j,V;Pe(A,1,`bg-(--color-surface-1) p-3 text-left hover:bg-(--color-surface-2) transition-colors
                 ${((V=(j=n(o))==null?void 0:j.area)==null?void 0:V.id)===n(v).id?"ring-1 ring-(--color-accent) ring-inset":""}`),P(O,n(v).name),P(T,n(v).players??0)}),Z("click",A,()=>i(n(v).id)),m(x,A)});var p=_(y,2);{var k=x=>{var v=dd(),A=u(v),C=u(A),E=_(A,2),O=u(E);{var D=B=>{var J=id(),R=_(u(J)),j=u(R);F(()=>P(j,n(o).background.name)),m(B,J)};K(O,B=>{n(o).background&&B(D)})}var T=_(O,2);{var z=B=>{var J=ld(),R=_(u(J)),j=u(R);F(()=>P(j,n(o).music.name||"None")),m(B,J)};K(T,B=>{n(o).music&&B(z)})}var I=_(T,2);{var H=B=>{var J=cd(),R=_(u(J)),j=u(R);F(()=>P(j,`Def ${n(o).hp.defense??""} / Pro ${n(o).hp.prosecution??""}`)),m(B,J)};K(I,B=>{n(o).hp&&B(H)})}F(()=>{var B;return P(C,(B=n(o).area)==null?void 0:B.name)}),m(x,v)};K(p,x=>{n(o)&&x(k)})}m(g,b)};K(c,g=>{n(a)?g(d):g(f,-1)})}m(e,l),Ge()}Ut(["click"]);var pd=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),hd=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-medium"> </td><td class="px-4 py-1.5"><span> </span></td></tr>'),_d=w('<tr><td colspan="2" class="px-4 py-8 text-center text-(--color-text-muted)">No accounts</td></tr>'),xd=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">Username</th><th class="px-4 py-2">Role</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),bd=w('<div class="space-y-4"><h2 class="text-lg font-semibold">Accounts</h2> <!></div>');function md(e,t){Ke(t,!0);let r=W(me([])),a=W(!0);async function o(){var f;const d=await rt("/admin/users");d.status===200&&S(r,((f=d.data)==null?void 0:f.users)||[],!0),S(a,!1)}Je(()=>{o()});var s=bd(),i=_(u(s),2);{var l=d=>{var f=pd();m(d,f)},c=d=>{var f=xd(),g=u(f),b=u(g),y=_(u(b));Se(y,21,()=>n(r),ke,(p,k)=>{var x=hd(),v=u(x),A=u(v),C=_(v),E=u(C),O=u(E);F(()=>{P(A,n(k).username),Pe(E,1,`text-[10px] px-1 py-px font-medium
                    ${n(k).acl==="SUPER"?"bg-violet-500/15 text-violet-400":n(k).acl==="NONE"?"bg-(--color-surface-3) text-(--color-text-muted)":"bg-cyan-500/15 text-cyan-400"}`),P(O,n(k).acl)}),m(p,x)},p=>{var k=_d();m(p,k)}),m(d,f)};K(i,d=>{n(a)?d(l):d(c,-1)})}m(e,s),Ge()}var gd=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),yd=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-3 py-1.5 font-mono text-xs"> </td><td class="px-3 py-1.5 text-(--color-text-secondary) max-w-xs truncate"> </td><td class="px-3 py-1.5 text-xs"> </td></tr>'),wd=w('<table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-3 py-2">Target</th><th class="px-3 py-2">Reason</th><th class="px-3 py-2">Expires</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table>'),$d=w('<p class="text-sm text-(--color-text-muted)">No active rules.</p>'),kd=w('<div class="bg-(--color-surface-1) border border-(--color-border) p-4"><div class="flex items-center gap-2 mb-3"><span class="text-sm font-medium">nftables</span> <span> </span> <span class="text-xs text-(--color-text-muted)"> </span></div> <!></div>'),Sd=w('<tr class="hover:bg-(--color-surface-2)/50"><td class="px-4 py-1.5 font-mono text-xs"> </td><td class="px-4 py-1.5"> </td><td class="px-4 py-1.5"><span> </span></td><td class="px-4 py-1.5 text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td><td class="px-4 py-1.5 text-(--color-text-muted) hidden md:table-cell tabular-nums"> </td></tr>'),Ed=w('<tr><td colspan="5" class="px-4 py-8 text-center text-(--color-text-muted)">No flagged ASNs</td></tr>'),Ad=w('<div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="overflow-x-auto"><table class="w-full text-sm"><thead><tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)"><th class="px-4 py-2">ASN</th><th class="px-4 py-2">Organization</th><th class="px-4 py-2">Status</th><th class="px-4 py-2 hidden md:table-cell">Events</th><th class="px-4 py-2 hidden md:table-cell">IPs</th></tr></thead><tbody class="divide-y divide-(--color-border)/50"></tbody></table></div></div>'),Nd=w('<div class="space-y-4"><h2 class="text-lg font-semibold">Firewall</h2> <div class="flex gap-4 border-b border-(--color-border)"><button>IP Rules</button> <button>ASN Reputation</button></div> <!></div>');function Td(e,t){Ke(t,!0);let r=W("firewall"),a=W(me({enabled:!1,rules:[]})),o=W(me([])),s=W(!0);async function i(){S(s,!0);const v=await rt("/admin/firewall");v.status===200&&S(a,v.data,!0),S(s,!1)}async function l(){var A;S(s,!0);const v=await rt("/admin/asn-reputation");v.status===200&&S(o,((A=v.data)==null?void 0:A.asn_entries)||[],!0),S(s,!1)}Je(()=>{n(r)==="firewall"?i():l()});function c(v){return v?new Date(v*1e3).toLocaleString():"Never"}var d=Nd(),f=_(u(d),2),g=u(f),b=_(g,2),y=_(f,2);{var p=v=>{var A=gd();m(v,A)},k=v=>{var A=kd(),C=u(A),E=_(u(C),2),O=u(E),D=_(E,2),T=u(D),z=_(C,2);{var I=B=>{var J=wd(),R=_(u(J));Se(R,21,()=>n(a).rules,ke,(j,V)=>{var G=yd(),le=u(G),M=u(le),N=_(le),Y=u(N),ee=_(N),ce=u(ee);F(ae=>{P(M,n(V).target),P(Y,n(V).reason),P(ce,ae)},[()=>n(V).expires_at===0?"Permanent":c(n(V).expires_at)]),m(j,G)}),m(B,J)},H=B=>{var J=$d();m(B,J)};K(z,B=>{n(a).rules.length>0?B(I):B(H,-1)})}F(()=>{Pe(E,1,`text-[10px] px-1 py-px font-medium ${n(a).enabled?"bg-emerald-500/15 text-emerald-400":"bg-(--color-surface-3) text-(--color-text-muted)"}`),P(O,n(a).enabled?"Enabled":"Disabled"),P(T,`${n(a).rules.length??""} rules`)}),m(v,A)},x=v=>{var A=Ad(),C=u(A),E=u(C),O=_(u(E));Se(O,21,()=>n(o),ke,(D,T)=>{var z=Sd(),I=u(z),H=u(I),B=_(I),J=u(B),R=_(B),j=u(R),V=u(j),G=_(R),le=u(G),M=_(G),N=u(M);F(()=>{P(H,`AS${n(T).asn??""}`),P(J,n(T).as_org),Pe(j,1,`text-[10px] px-1 py-px font-medium
                    ${n(T).status==="blocked"?"bg-red-500/15 text-red-400":n(T).status==="rate_limited"?"bg-orange-500/15 text-orange-400":n(T).status==="watched"?"bg-amber-500/15 text-amber-400":"bg-(--color-surface-3) text-(--color-text-muted)"}`),P(V,n(T).status),P(le,n(T).total_abuse_events),P(N,n(T).abusive_ips)}),m(D,z)},D=>{var T=Ed();m(D,T)}),m(v,A)};K(y,v=>{n(s)?v(p):n(r)==="firewall"?v(k,1):v(x,-1)})}F(()=>{Pe(g,1,`pb-2 text-sm border-b-2 transition-colors ${n(r)==="firewall"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`),Pe(b,1,`pb-2 text-sm border-b-2 transition-colors ${n(r)==="asn"?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`)}),Z("click",g,()=>S(r,"firewall")),Z("click",b,()=>S(r,"asn")),m(e,d),Ge()}Ut(["click"]);var Pd=w('<pre class="text-xs text-emerald-400 bg-emerald-400/10 border border-emerald-400/20 px-3 py-2 whitespace-pre-wrap"> </pre>'),Id=w("<button> </button>"),Md=w('<p class="text-(--color-text-muted) text-sm">Loading...</p>'),Od=w('<div class="px-4 py-2 bg-(--color-surface-2) text-xs font-semibold uppercase tracking-wider text-(--color-text-secondary) border-b border-(--color-border) sticky top-0 flex items-center justify-between"><span> </span> <button class="p-0.5 text-(--color-text-muted) hover:text-red-400"><!></button></div>'),Cd=w('<div class="px-2 py-1 flex items-center gap-1 text-sm hover:bg-(--color-surface-2)/50 border-b border-(--color-border)/30 group"><span class="text-[10px] text-(--color-text-muted) w-6 text-right tabular-nums shrink-0"></span> <div class="flex flex-col shrink-0 opacity-0 group-hover:opacity-100 transition-opacity"><button class="text-[8px] text-(--color-text-muted) hover:text-(--color-text-primary) leading-none">&blacktriangle;</button> <button class="text-[8px] text-(--color-text-muted) hover:text-(--color-text-primary) leading-none">&blacktriangledown;</button></div> <span class="flex-1 truncate"> </span> <button class="p-0.5 text-(--color-text-muted) hover:text-red-400 opacity-0 group-hover:opacity-100 transition-opacity"><!></button></div>'),Ld=w('<div class="px-4 py-8 text-center text-(--color-text-muted) text-sm">No items</div>'),Rd=w(`<div class="flex gap-px"><input type="text" class="flex-1 px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
               placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active)"/> <button class="px-3 py-1.5 text-sm bg-(--color-surface-3) border border-(--color-border) text-(--color-text-secondary) hover:text-(--color-text-primary)"><!></button></div> <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden"><div class="max-h-[60vh] overflow-y-auto"></div></div>`,1),Dd=w(`<div class="space-y-3"><div class="flex items-center justify-between flex-wrap gap-2"><h2 class="text-lg font-semibold">Content</h2> <button class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-(--color-accent) text-(--color-surface-0)
             hover:opacity-80 disabled:opacity-30"><!> </button></div> <!> <div class="flex gap-4 border-b border-(--color-border)"></div> <!></div>`);function zd(e,t){Ke(t,!0);let r=W("areas"),a=W(me({characters:[],music:[],areas:[]})),o=W(me({characters:[],music:[],areas:[]})),s=W(!0),i=W(!1),l=W(""),c=W("");async function d(){const R=await rt("/admin/content");R.status===200&&(S(a,R.data,!0),S(o,JSON.parse(JSON.stringify(R.data)),!0)),S(s,!1)}Je(()=>{d()});function f(){return n(r)==="areas"?n(a).areas:n(r)==="characters"?n(a).characters:n(r)==="music"?n(a).music:[]}function g(R){n(r)==="areas"?n(a).areas=R:n(r)==="characters"?n(a).characters=R:n(r)==="music"&&(n(a).music=R)}function b(R){return n(r)==="music"&&!R.includes(".")}function y(){return JSON.stringify(n(a))!==JSON.stringify(n(o))}function p(){if(!n(c).trim())return;const R=[...f(),n(c).trim()];g(R),S(c,"")}function k(R){const j=[...f()];j.splice(R,1),g(j)}function x(R,j){const V=[...f()],G=R+j;G<0||G>=V.length||([V[R],V[G]]=[V[G],V[R]],g(V))}async function v(){var j,V;if(!y()){S(l,"No changes.");return}S(i,!0),S(l,"");const R=await _a("PUT","/admin/content",n(a));S(i,!1),R.status===200?(S(l,((j=R.data)==null?void 0:j.reload_summary)||"Content saved and applied.",!0),S(o,JSON.parse(JSON.stringify(n(a))),!0)):S(l,((V=R.data)==null?void 0:V.reason)||"Failed to save",!0)}var A=Dd(),C=u(A),E=_(u(C),2),O=u(E);es(O,{size:13,strokeWidth:1.5});var D=_(O),T=_(C,2);{var z=R=>{var j=Pd(),V=u(j);F(()=>P(V,n(l))),m(R,j)};K(T,R=>{n(l)&&R(z)})}var I=_(T,2);Se(I,20,()=>[["areas","Areas"],["characters","Characters"],["music","Music"]],ke,(R,j)=>{var V=pe(()=>ho(j,2));let G=()=>n(V)[0],le=()=>n(V)[1];var M=Id(),N=u(M);F(()=>{var Y;Pe(M,1,`pb-2 text-sm border-b-2 transition-colors ${n(r)===G()?"border-(--color-accent) text-(--color-accent)":"border-transparent text-(--color-text-muted)"}`),P(N,`${le()??""} (${((Y=n(a)[G()])==null?void 0:Y.length)??0??""})`)}),Z("click",M,()=>{S(r,G(),!0),S(c,"")}),m(R,M)});var H=_(I,2);{var B=R=>{var j=Md();m(R,j)},J=R=>{var j=Rd(),V=ie(j),G=u(V),le=_(G,2),M=u(le);Qn(M,{size:14,strokeWidth:1.5});var N=_(V,2),Y=u(N);Se(Y,21,f,ke,(ee,ce,ae)=>{var $=ue(),L=ie($);{var q=Te=>{var xe=Od(),Me=u(xe),fe=u(Me),ve=_(Me,2),be=u(ve);uo(be,{size:11,strokeWidth:1.5}),F(()=>P(fe,n(ce))),Z("click",ve,()=>k(ae)),m(Te,xe)},Q=pe(()=>b(n(ce))),_e=Te=>{var xe=Cd(),Me=u(xe);Me.textContent=ae+1;var fe=_(Me,2),ve=u(fe),be=_(ve,2),oe=_(fe,2),He=u(oe),Ye=_(oe,2),Ot=u(Ye);uo(Ot,{size:12,strokeWidth:1.5}),F(()=>P(He,n(ce))),Z("click",ve,()=>x(ae,-1)),Z("click",be,()=>x(ae,1)),Z("click",Ye,()=>k(ae)),m(Te,xe)};K(L,Te=>{n(Q)?Te(q):Te(_e,-1)})}m(ee,$)},ee=>{var ce=Ld();m(ee,ce)}),F(ee=>Ta(G,"placeholder",`Add ${ee??""}...`),[()=>n(r)==="music"?"track or category":n(r).slice(0,-1)]),Z("keydown",G,ee=>ee.key==="Enter"&&p()),nr(G,()=>n(c),ee=>S(c,ee)),Z("click",le,p),m(R,j)};K(H,R=>{n(s)?R(B):R(J,-1)})}F(R=>{E.disabled=R,P(D,` ${n(i)?"Saving...":"Save & Apply"}`)},[()=>n(i)||!y()]),Z("click",E,v),m(e,A),Ge()}Ut(["click","keydown"]);var jd=w('<div class="text-center text-gray-500 mt-20"><h2 class="text-2xl font-semibold mb-2">Coming Soon</h2> <p>The <code class="text-gray-400"> </code> page is not yet implemented.</p></div>'),Fd=w('<div class="flex h-screen bg-(--color-surface-0) text-(--color-text-primary)"><!> <main class="flex-1 overflow-auto p-4 md:p-6 lg:p-8"><!></main></div>');function Bd(e,t){Ke(t,!0);let r=W(me(window.location.hash||"#/login"));function a(){S(r,window.location.hash||"#/login",!0)}Je(()=>(window.addEventListener("hashchange",a),()=>window.removeEventListener("hashchange",a))),Je(()=>(se.loggedIn?il():Jo(),()=>Jo()));let o=pe(()=>!se.loggedIn&&n(r)!=="#/login"?(window.location.hash="#/login","login"):se.loggedIn&&n(r)==="#/login"?(window.location.hash="#/dashboard","dashboard"):n(r).slice(2)||"login");var s=ue(),i=ie(s);{var l=d=>{Rl(d,{})},c=d=>{var f=Fd(),g=u(f);Ml(g,{get currentPage(){return n(o)}});var b=_(g,2),y=u(b);{var p=I=>{Gl(I,{})},k=I=>{Ac(I,{})},x=I=>{Cc(I,{})},v=I=>{jc(I,{})},A=I=>{Uc(I,{})},C=I=>{td(I,{})},E=I=>{vd(I,{})},O=I=>{md(I,{})},D=I=>{Td(I,{})},T=I=>{zd(I,{})},z=I=>{var H=jd(),B=_(u(H),2),J=_(u(B)),R=u(J);F(()=>P(R,n(o))),m(I,H)};K(y,I=>{n(o)==="dashboard"?I(p):n(o)==="config"?I(k,1):n(o)==="sessions"?I(x,2):n(o)==="traffic"?I(v,3):n(o)==="bans"?I(A,4):n(o)==="moderation"?I(C,5):n(o)==="areas"?I(E,6):n(o)==="users"?I(O,7):n(o)==="firewall"?I(D,8):n(o)==="content"?I(T,9):I(z,-1)})}m(d,f)};K(i,d=>{se.loggedIn?d(c,-1):d(l)})}m(e,s),Ge()}Mi(Bd,{target:document.getElementById("app")});
